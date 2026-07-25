#include "ff.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "fs_custom.h"
#include "FileHandler.h"
#include "CanLogManager.h"
#include "CanLogBuffer.h"
#include "httpd_post.h"
#include "WebInterface.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-unknown"
#endif

struct fs_custom_data
{
    FILE *f;
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
    int delay_read;
    fs_wait_cb callback_fn;
    void *callback_arg;
#endif
};

static const char redirect_reply[] = "HTTP/1.1 303 See Other\r\n"
                                     "Location: /can.shtml\r\n"
                                     "Connection: close\r\n"
                                     "Content-Length: 0\r\n"
                                     "\r\n";

#if LWIP_HTTPD_CUSTOM_FILES

#if LWIP_HTTPD_DYNAMIC_FILE_READ != 1
#warning "LWIP_HTTPD_DYNAMIC_FILE_READ is NOT enabled!"
#endif

#define CANLOG_MAX_PATH_LENGTH    64U
#define CANLOG_MAX_META_DATA_SIZE 96U
#define CANLOG_MAX_STATUS_SIZE    768U
#define CANLOG_MAX_CONFIG_SIZE    96U
#define IAP_STATUS_MAX_SIZE       640U
#define CANLOG_FILE_PATH          "/logs/CAN.LOG"
#define CANLOG_META_DATA_PATH     "/logs/meta"
#define CANLOG_CONFIG_PATH        "/logs/config"
#define CANLOG_STATUS_PATH        "/logger/status"
#define CANLOG_POST_REDIRECT_PATH "/postredir"
#define IAP_STATUS_PATH           "/iap/status"
#define IAP_PREPARE_PATH          "/iap/prepare"
#define IAP_APPLY_PATH            "/iap/apply"
#define APP_VERSION_PATH          "/api/version"

#define IAP_VERIFY_STATE_IDLE      ((uint8_t)0u)
#define IAP_VERIFY_STATE_PENDING   ((uint8_t)1u)
#define IAP_VERIFY_STATE_VERIFYING ((uint8_t)2u)
#define IAP_VERIFY_STATE_VERIFIED  ((uint8_t)3u)
#define IAP_VERIFY_STATE_ERROR     ((uint8_t)4u)

#define CANLOG_META_DATA_STRING                                                \
    "{\"head\":%lu,\"tail\":%lu,\"capacity\":%lu,\"latest\":\"CAN.LOG%lu\","   \
    "\"file_size\":\"%lu\"}"

#define CANLOG_STATUS_STRING                                                   \
    "{ "                                                                       \
    "\"active\":%s,\"frames_lost\":%s,\"prealloc_errors\":%s,\"frames_total_"  \
    "hi\":%lu,\"frames_total_lo\":%lu,\"bus_load_1\":%.2f,\"bus_load_2\":%."   \
    "2f,\"rb1_bytes_highwater_pct\":%.2f,\"can1_rx_highwater_pct\":%.2f,"      \
    "\"can2_rx_highwater_pct\":%.2f,\"fdcan_msg_port_highwater_pct\":%.2f,"    \
    "\"sd_write_max_us\":%lu,\"sd_sync_max_us\":%lu,\"sd_store_block_max_"     \
    "us\":"                                                                    \
    "%lu,\"can1_tx_replay_requests\":%lu,\"can1_tx_replay_completed\":%lu,"    \
    "\"can1_tx_replay_irqs\":%lu,\"can2_tx_replay_requests\":%lu,"             \
    "\"can2_tx_replay_completed\":%lu,\"can2_tx_replay_irqs\":%lu}"

#define CANLOG_CONFIG_STRING                                                   \
    "{\"cluster_size\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}"

#define IAP_STATUS_STRING                                                      \
    "{\"app_version\":\"%s\",\"logger_active\":%s,\"upload_ready\":%s,"        \
    "\"upload_state\":\"%s\","                                                 \
    "\"upload_received\":%lu,\"upload_total\":%lu,\"received\":%lu,"           \
    "\"total\":%lu,\"verify_state\":\"%s\","                                   \
    "\"verify_processed\":%lu,\"verify_total\":%lu,\"verified\":%s,"           \
    "\"apply_ready\":%s,\"error_reason\":\"%s\",\"ingest_status\":%lu}"

#define IAP_APPLY_RESULT_STRING   "{\"ok\":%s,\"reason\":\"%s\"}"
#define IAP_PREPARE_RESULT_STRING "{\"ok\":%s,\"reason\":\"%s\"}"
#define APP_VERSION_STRING        "{\"app_version\":\"%s\"}"

typedef struct
{
    uint8_t stage;
    uint32_t index;
    uint32_t callcount;
    uint32_t byteCount;
    char name[CANLOG_MAX_PATH_LENGTH];
} CustomHandlerState;

static CustomHandlerState reqState;
static FatFsDeviceType CanLogReadFileDevice;

/* WARNING: Not thread-safe. MetaData/StatusData shared across requests.
 * Assumes single-threaded or serialized HTTP request processing. */
static char MetaData[CANLOG_MAX_META_DATA_SIZE];
static char StatusData[CANLOG_MAX_STATUS_SIZE];
static char ConfigData[CANLOG_MAX_CONFIG_SIZE];
static char IapStatusData[IAP_STATUS_MAX_SIZE];
static char IapPrepareResultData[96U];
static char IapApplyResultData[96U];
static char AppVersionData[96U];

int fs_open_custom(struct fs_file *file, const char *name)
{
    uint32_t FileSize = 0U;

    /* accept only files inside /logs/ and beginning with CAN.LOG ---- */
    if (0 == strncmp(name, CANLOG_FILE_PATH, sizeof(CANLOG_FILE_PATH) - 1))
    {
        char FileName[CANLOG_MAX_PATH_LENGTH] = FILEHANDLER_PARTITION_NO;

        reqState.index     = 0;
        reqState.stage     = 0;
        reqState.callcount = 0;
        reqState.byteCount = 0;

        strncat(FileName, name, sizeof(FileName) - strlen(FileName) - 1);
        strncpy((char *)reqState.name, FileName, sizeof(reqState.name) - 1);
        reqState.name[sizeof(reqState.name) - 1] = '\0';

        if (0 != FatFS_SD_OpenFileForRead(&CanLogReadFileDevice, FileName))
        {
            return 0;
        }

        (void)FatFS_SD_GetFileSize(&CanLogReadFileDevice, &FileSize);

        CanLogReadFileDevice.readTargetSize = FileSize;

        file->pextension     = &reqState;
        file->data           = NULL;
        file->len            = FileSize; // tell lwip the total file size
        file->index          = 0;
        file->is_custom_file = 1;

        return 1;
    }
    else if (0
             == strncmp(
                 name,
                 CANLOG_META_DATA_PATH,
                 sizeof(CANLOG_META_DATA_PATH) - 1
             ))
    {
        uint32_t HeadIndex;
        uint32_t TailIndex;
        uint32_t Capacity;
        uint32_t Progression;
        int DataSize;

        (void)FsCustom_GetCanLogHeadIndex(&HeadIndex);
        (void)FsCustom_GetCanLogTailIndex(&TailIndex);
        (void)FsCustom_GetCanLogCapacity(&Capacity);

        if (HeadIndex >= TailIndex)
        {
            Progression = HeadIndex - TailIndex + 1;
        }
        else if (HeadIndex < TailIndex)
        {
            Progression = Capacity + (HeadIndex - TailIndex + 1);
        }

        if (Progression < 2)
        {
            HeadIndex = TailIndex;
        }

        DataSize = snprintf(
            MetaData,
            sizeof MetaData,
            CANLOG_META_DATA_STRING,
            (unsigned long int)HeadIndex,
            (unsigned long int)TailIndex,
            (unsigned long int)Capacity,
            (unsigned long int)((HeadIndex + Capacity - 1) % Capacity),
            (unsigned long int)(appCanLogGetLogFileSize())
        );

        if (DataSize < 0 || (size_t)DataSize >= sizeof(MetaData))
        {
            return 0; // Error: formatting failed or buffer too small
        }

        file->data           = MetaData;
        file->len            = DataSize;
        file->index          = 0;
        file->is_custom_file = 0; /* httpd sends static buffer     */
        return 1;
    }
    else if (0
             == strncmp(
                 name,
                 CANLOG_CONFIG_PATH,
                 sizeof(CANLOG_CONFIG_PATH) - 1
             ))
    {
        int DataSize;

        DataSize = snprintf(
            ConfigData,
            sizeof ConfigData,
            CANLOG_CONFIG_STRING,
            (unsigned long int)(appCanLogGetClusterSize()),
            (unsigned long int)(appCanLogGetLogFileSize()),
            (unsigned long int)(appCanLogGetLogFileCount())
        );

        if (DataSize < 0 || (size_t)DataSize >= sizeof(ConfigData))
        {
            return 0; // Error: formatting failed or buffer too small
        }

        file->data           = ConfigData;
        file->len            = DataSize;
        file->index          = 0;
        file->is_custom_file = 0; /* httpd sends static buffer     */
        return 1;
    }
    else if (0
             == strncmp(
                 name,
                 CANLOG_STATUS_PATH,
                 sizeof(CANLOG_STATUS_PATH) - 1
             ))
    {
        uint8_t IsTracerRunning        = 1;
        float BusLoadCan1              = 0.0;
        float BusLoadCan2              = 0.0;
        _Bool AnyFrameLost             = false;
        uint8_t PreallocErrors         = 0U;
        uint32_t Rb1BytesHighWater     = 0U;
        float Rb1BytesHighWaterPct     = 0.0f;
        uint32_t CanAbsRxHighWaterCan1 = 0U;
        uint32_t CanAbsRxHighWaterCan2 = 0U;
        uint32_t CanAbsRxCapacity      = 0U;
        float CanAbsRxHighWaterPctCan1 = 0.0f;
        float CanAbsRxHighWaterPctCan2 = 0.0f;
        uint32_t FdcanMsgPortHighWater = 0U;
        uint32_t FdcanMsgPortCapacity  = 0U;
        float FdcanMsgPortHighWaterPct = 0.0f;
        uint64_t FrameCount            = 0U;
        unsigned long FrameCountHi     = 0UL;
        unsigned long FrameCountLo     = 0UL;
        uint32_t SdWriteMaxUs          = 0U;
        uint32_t SdSyncMaxUs           = 0U;
        uint32_t SdStoreBlockMaxUs     = 0U;
        uint32_t Can1TxReplayRequests  = 0U;
        uint32_t Can1TxReplayCompleted = 0U;
        uint32_t Can1TxReplayIrqs      = 0U;
        uint32_t Can2TxReplayRequests  = 0U;
        uint32_t Can2TxReplayCompleted = 0U;
        uint32_t Can2TxReplayIrqs      = 0U;

        if (0 != FsCustom_IsTracerRunning(&IsTracerRunning))
        {
            IsTracerRunning = 1;
        }

        AnyFrameLost = FsCustom_IsAnyFrameLostFlag();
        (void)FsCustom_GetPreallocErrorFlag(&PreallocErrors);
        FsCustom_GetBusloadCan1(&BusLoadCan1);
        FsCustom_GetBusloadCan2(&BusLoadCan2);
        if (0U != FsCustom_GetRb1BytesHighWater(&Rb1BytesHighWater))
        {
            Rb1BytesHighWater = 0U;
        }
        if (0U != FsCustom_GetCanAbsRxHighWaterCan1(&CanAbsRxHighWaterCan1))
        {
            CanAbsRxHighWaterCan1 = 0U;
        }
        if (0U != FsCustom_GetCanAbsRxHighWaterCan2(&CanAbsRxHighWaterCan2))
        {
            CanAbsRxHighWaterCan2 = 0U;
        }
        if (0U != FsCustom_GetCanAbsRxCapacity(&CanAbsRxCapacity))
        {
            CanAbsRxCapacity = 0U;
        }
        if (0U != FsCustom_GetFdcanMsgPortHighWater(&FdcanMsgPortHighWater))
        {
            FdcanMsgPortHighWater = 0U;
        }
        if (0U != FsCustom_GetFdcanMsgPortCapacity(&FdcanMsgPortCapacity))
        {
            FdcanMsgPortCapacity = 0U;
        }
        if (0U != FsCustom_GetCanLogFrameCount(&FrameCount))
        {
            FrameCount = 0U;
        }
        if (0U
            != FsCustom_GetCanLogSdTimingMaxUs(
                &SdWriteMaxUs,
                &SdSyncMaxUs,
                &SdStoreBlockMaxUs
            ))
        {
            SdWriteMaxUs      = 0U;
            SdSyncMaxUs       = 0U;
            SdStoreBlockMaxUs = 0U;
        }
        if (0U
            != FsCustom_GetStaticTxReplayStatsCan1(
                &Can1TxReplayRequests,
                &Can1TxReplayCompleted,
                &Can1TxReplayIrqs
            ))
        {
            Can1TxReplayRequests  = 0U;
            Can1TxReplayCompleted = 0U;
            Can1TxReplayIrqs      = 0U;
        }
        if (0U
            != FsCustom_GetStaticTxReplayStatsCan2(
                &Can2TxReplayRequests,
                &Can2TxReplayCompleted,
                &Can2TxReplayIrqs
            ))
        {
            Can2TxReplayRequests  = 0U;
            Can2TxReplayCompleted = 0U;
            Can2TxReplayIrqs      = 0U;
        }
        FrameCountHi = (unsigned long)((FrameCount >> 32) & 0xFFFFFFFFULL);
        FrameCountLo = (unsigned long)(FrameCount & 0xFFFFFFFFULL);

        if (LOG_BUFFER_SIZE > 0U)
        {
            Rb1BytesHighWaterPct =
                (float)Rb1BytesHighWater * 100.0f / (float)LOG_BUFFER_SIZE;
        }
        if (CanAbsRxCapacity > 0U)
        {
            CanAbsRxHighWaterPctCan1 =
                (float)CanAbsRxHighWaterCan1 * 100.0f / (float)CanAbsRxCapacity;
            CanAbsRxHighWaterPctCan2 =
                (float)CanAbsRxHighWaterCan2 * 100.0f / (float)CanAbsRxCapacity;
        }
        if (FdcanMsgPortCapacity > 0U)
        {
            FdcanMsgPortHighWaterPct = (float)FdcanMsgPortHighWater * 100.0f
                                       / (float)FdcanMsgPortCapacity;
        }

        int n = snprintf(
            StatusData,
            sizeof(StatusData),
            CANLOG_STATUS_STRING,
            IsTracerRunning ? "true" : "false",
            AnyFrameLost ? "true" : "false",
            PreallocErrors ? "true" : "false",
            FrameCountHi,
            FrameCountLo,
            BusLoadCan1,
            BusLoadCan2,
            Rb1BytesHighWaterPct,
            CanAbsRxHighWaterPctCan1,
            CanAbsRxHighWaterPctCan2,
            FdcanMsgPortHighWaterPct,
            (unsigned long)SdWriteMaxUs,
            (unsigned long)SdSyncMaxUs,
            (unsigned long)SdStoreBlockMaxUs,
            (unsigned long)Can1TxReplayRequests,
            (unsigned long)Can1TxReplayCompleted,
            (unsigned long)Can1TxReplayIrqs,
            (unsigned long)Can2TxReplayRequests,
            (unsigned long)Can2TxReplayCompleted,
            (unsigned long)Can2TxReplayIrqs
        );

        if (n < 0 || (size_t)n >= sizeof(StatusData))
        {
            return 0;
        }

        file->data           = StatusData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0; /* httpd sends static buffer     */
        return 1;
    }
    else if (0 == strncmp(name, IAP_STATUS_PATH, sizeof(IAP_STATUS_PATH) - 1))
    {
        uint8_t is_logger_active      = 1u;
        uint8_t upload_state          = HTTPD_IAP_UPLOAD_STATE_IDLE;
        uint8_t upload_ready          = 0u;
        uint8_t verify_state          = IAP_VERIFY_STATE_IDLE;
        uint8_t is_verified           = 0u;
        uint8_t apply_ready           = 0u;
        uint8_t upload_error          = HTTPD_IAP_UPLOAD_ERROR_NONE;
        uint8_t ingest_status         = 0u;
        uint32_t upload_received      = 0u;
        uint32_t upload_total         = 0u;
        uint32_t verify_processed     = 0u;
        uint32_t verify_total         = 0u;
        const char *upload_state_name = "idle";
        const char *verify_state_name = "idle";
        const char *error_reason_name = "none";
        int n                         = 0;

        if (0 != FsCustom_IsTracerRunning(&is_logger_active))
        {
            is_logger_active = 1u;
        }

        HttpdPost_GetIapUploadState(
            &upload_state,
            &upload_received,
            &upload_total
        );
        HttpdPost_GetIapUploadError(&upload_error, &ingest_status);
        upload_ready = HttpdPost_IsIapUploadReady();
        WebInterface_GetFirmwareVerifyStatusHook(
            &verify_state,
            &verify_processed,
            &verify_total
        );
        is_verified = WebInterface_IsFirmwareVerifiedHook();
        apply_ready = (is_logger_active == 0u && is_verified != 0u) ? 1u : 0u;

        switch (upload_state)
        {
        case HTTPD_IAP_UPLOAD_STATE_IN_PROGRESS:
            upload_state_name = "in_progress";
            break;
        case HTTPD_IAP_UPLOAD_STATE_READY:
            upload_state_name = "ready";
            break;
        case HTTPD_IAP_UPLOAD_STATE_ERROR:
            upload_state_name = "error";
            break;
        case HTTPD_IAP_UPLOAD_STATE_IDLE:
        default:
            upload_state_name = "idle";
            break;
        }

        switch (verify_state)
        {
        case IAP_VERIFY_STATE_PENDING:
            verify_state_name = "pending";
            break;
        case IAP_VERIFY_STATE_VERIFYING:
            verify_state_name = "in_progress";
            break;
        case IAP_VERIFY_STATE_VERIFIED:
            verify_state_name = "verified";
            break;
        case IAP_VERIFY_STATE_ERROR:
            verify_state_name = "error";
            break;
        case IAP_VERIFY_STATE_IDLE:
        default:
            verify_state_name = "idle";
            break;
        }

        switch (upload_error)
        {
        case HTTPD_IAP_UPLOAD_ERROR_BEGIN:
            error_reason_name = "begin";
            break;
        case HTTPD_IAP_UPLOAD_ERROR_PUSH:
            error_reason_name = "push";
            break;
        case HTTPD_IAP_UPLOAD_ERROR_INCOMPLETE:
            error_reason_name = "incomplete";
            break;
        case HTTPD_IAP_UPLOAD_ERROR_FINISH:
            error_reason_name = "finish";
            break;
        case HTTPD_IAP_UPLOAD_ERROR_NONE:
        default:
            error_reason_name = "none";
            break;
        }

        n = snprintf(
            IapStatusData,
            sizeof(IapStatusData),
            IAP_STATUS_STRING,
            APP_VERSION,
            is_logger_active ? "true" : "false",
            upload_ready ? "true" : "false",
            upload_state_name,
            (unsigned long)upload_received,
            (unsigned long)upload_total,
            (unsigned long)upload_received,
            (unsigned long)upload_total,
            verify_state_name,
            (unsigned long)verify_processed,
            (unsigned long)verify_total,
            is_verified ? "true" : "false",
            apply_ready ? "true" : "false",
            error_reason_name,
            (unsigned long)ingest_status
        );
        if (n < 0 || (size_t)n >= sizeof(IapStatusData))
        {
            n = snprintf(
                IapStatusData,
                sizeof(IapStatusData),
                "{"
                "\"logger_active\":%s,"
                "\"upload_ready\":%s,"
                "\"upload_state\":\"error\","
                "\"upload_received\":%lu,"
                "\"upload_total\":%lu,"
                "\"received\":%lu,"
                "\"total\":%lu,"
                "\"verify_state\":\"error\","
                "\"verify_processed\":%lu,"
                "\"verify_total\":%lu,"
                "\"verified\":%s,"
                "\"apply_ready\":%s,"
                "\"error_reason\":\"status_payload_too_large\","
                "\"ingest_status\":%lu"
                "}",
                is_logger_active ? "true" : "false",
                upload_ready ? "true" : "false",
                (unsigned long)upload_received,
                (unsigned long)upload_total,
                (unsigned long)upload_received,
                (unsigned long)upload_total,
                (unsigned long)verify_processed,
                (unsigned long)verify_total,
                is_verified ? "true" : "false",
                apply_ready ? "true" : "false",
                (unsigned long)ingest_status
            );
            if (n < 0 || (size_t)n >= sizeof(IapStatusData))
            {
                return 0;
            }
        }

        file->data           = IapStatusData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0;
        return 1;
    }
    else if (0 == strncmp(name, APP_VERSION_PATH, sizeof(APP_VERSION_PATH) - 1))
    {
        int n = snprintf(
            AppVersionData,
            sizeof(AppVersionData),
            APP_VERSION_STRING,
            APP_VERSION
        );

        if (n < 0 || (size_t)n >= sizeof(AppVersionData))
        {
            return 0;
        }

        file->data           = AppVersionData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0;
        return 1;
    }
    else if (0 == strncmp(name, IAP_PREPARE_PATH, sizeof(IAP_PREPARE_PATH) - 1))
    {
        uint8_t is_logger_active = 1u;
        const char *reason       = "internal_error";
        const char *ok_text      = "false";
        int n                    = 0;

        if (0 != FsCustom_IsTracerRunning(&is_logger_active))
        {
            is_logger_active = 1u;
        }

        if (0u != is_logger_active)
        {
            reason = "logger_running";
        }
        else
        {
            HttpdPost_ResetIapUploadSession();

            if (0u == WebInterface_PrepareFirmwareUploadHook())
            {
                reason = "prepare_failed";
            }
            else
            {
                ok_text = "true";
                reason  = "slot_erased";
            }
        }

        n = snprintf(
            IapPrepareResultData,
            sizeof(IapPrepareResultData),
            IAP_PREPARE_RESULT_STRING,
            ok_text,
            reason
        );
        if (n < 0 || (size_t)n >= sizeof(IapPrepareResultData))
        {
            return 0;
        }

        file->data           = IapPrepareResultData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0;
        return 1;
    }
    else if (0 == strncmp(name, IAP_APPLY_PATH, sizeof(IAP_APPLY_PATH) - 1))
    {
        uint8_t is_logger_active = 1u;
        uint8_t is_verified      = 0u;
        const char *reason       = "internal_error";
        const char *ok_text      = "false";
        int n                    = 0;

        if (0 != FsCustom_IsTracerRunning(&is_logger_active))
        {
            is_logger_active = 1u;
        }

        is_verified = WebInterface_IsFirmwareVerifiedHook();

        if (0u != is_logger_active)
        {
            reason = "logger_running";
        }
        else if (0u == is_verified)
        {
            reason = "image_not_verified";
        }
        else
        {
            WebInterface_RequestFirmwareApplyHook();
            HttpdPost_ClearIapUploadReady();
            ok_text = "true";
            reason  = "reboot_requested";
        }

        n = snprintf(
            IapApplyResultData,
            sizeof(IapApplyResultData),
            IAP_APPLY_RESULT_STRING,
            ok_text,
            reason
        );
        if (n < 0 || (size_t)n >= sizeof(IapApplyResultData))
        {
            return 0;
        }

        file->data           = IapApplyResultData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0;
        return 1;
    }
    else if (0
             == strncmp(
                 name,
                 CANLOG_POST_REDIRECT_PATH,
                 sizeof(CANLOG_POST_REDIRECT_PATH) - 1
             ))
    {
        file->data = redirect_reply;
        file->len  = sizeof(redirect_reply) - 1;
        file->flags =
            FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT;
        file->index = 0;
#if LWIP_HTTPD_DYNAMIC_HEADERS
        // file->http_header_included = 1;
#endif
#if LWIP_HTTPD_CUSTOM_FILES
        file->is_custom_file = 1;
#endif
        return 1;
    }

    return 0; // Fallback to default file system
}

#define CAN_LOG_BUFFER_SIZE 8U

void fs_state_free(struct fs_file *file, void *state)
{
    LWIP_UNUSED_ARG(file);
    if (state != NULL)
    {
        if (state == &reqState)
        {
            (void)FatFS_SD_CloseFile(&CanLogReadFileDevice);
        }
    }
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    FRESULT fr;
    uint32_t len              = 0U;
    CustomHandlerState *state = (CustomHandlerState *)file->pextension;
    uint32_t ByteCount;

    if (state == NULL)
    {
        return FS_READ_EOF;
    }

    ByteCount = state->byteCount;

    if (ByteCount >= CanLogReadFileDevice.readTargetSize)
    {
        state->callcount = 0;
        state->byteCount = 0;
        return FS_READ_EOF;
    }

    if (count <= 0)
    {
        return FS_READ_EOF;
    }
    else if (CanLogReadFileDevice.readTargetSize - ByteCount > (uint32_t)count)
    {
        len = count;
    }
    else
    {
        len = CanLogReadFileDevice.readTargetSize - ByteCount;
    }

    fr = FatFS_SD_ReadFile(&CanLogReadFileDevice, buffer, len);

    if (FR_OK != fr)
    {
        return FS_READ_EOF;
    }

    state->callcount++;
    state->byteCount += len;

    return len; // triggers send
}

void fs_close_custom(struct fs_file *file)
{
    FatFS_SD_CloseFile(&CanLogReadFileDevice);
    file->pextension = NULL; // optional cleanup
}

#endif
