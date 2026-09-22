#include "ff.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>

#include "fs_custom.h"
#include "fs_custom_internal.h"
#include "FileHandler.h"
#include "CanLogManager.h"
#include "CanLogBuffer.h"
#include "ErrorRecorder.h"

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
#define CANLOG_MAX_META_DATA_SIZE 128U
#define CANLOG_MAX_STATUS_SIZE    5120U
#define CANLOG_MAX_DEBUG_SIZE     4096U
#define CANLOG_MAX_CONFIG_SIZE    96U
#define IAP_STATUS_MAX_SIZE       640U
#define CANLOG_FILE_PATH          "/logs/CAN.LOG"
#define CANLOG_META_DATA_PATH     "/logs/meta"
#define CANLOG_CONFIG_PATH        "/logs/config"
#define CANLOG_STATUS_PATH        "/logger/status"
#define CANLOG_DEBUG_PATH         "/logger/debug"
#define ERRORRECORDER_PATH        "/logger/errors"
#define CANLOG_POST_REDIRECT_PATH "/postredir"
#define IAP_STATUS_PATH           "/iap/status"
#define IAP_PREPARE_PATH          "/iap/prepare"
#define IAP_APPLY_PATH            "/iap/apply"
#define APP_VERSION_PATH          "/api/version"
#define APP_CARD_PATH             "/api/card"

#define CANLOG_META_DATA_STRING                                                \
    "{\"head\":%lu,\"tail\":%lu,\"capacity\":%lu,\"latest\":\"CAN.LOG%lu\","   \
    "\"file_size\":\"%lu\"}"

#define CANLOG_STATUS_STRING                                                   \
    "{ "                                                                       \
    "\"active\":%s,\"frames_lost\":%s,\"prealloc_errors\":%s,\"frames_total_"  \
    "hi\":%lu,\"frames_total_lo\":%lu,\"bus_load_1\":%.2f,\"bus_load_2\":%."   \
    "2f,\"rb1_bytes_capacity\":%lu,\"rb1_bytes_now\":%lu,\"rb1_bytes_"         \
    "highwater\":%lu,\"rb1_bytes_highwater_pct\":%.2f,\"rb1_bytes_min_"        \
    "since_status\":%lu,\"rb1_bytes_max_since_status\":%lu,\"rb1_bytes_at_"    \
    "last_sd_block_start\":%lu,\"rb1_bytes_at_last_sd_block_end_before_"       \
    "consume\":%lu,\"rb1_bytes_at_last_sd_block_end_after_consume\":%lu,"      \
    "\"sd_block_attempts_since_status\":%lu,\"sd_blocks_written_since_"        \
    "status\":%lu,\"sd_block_errors_since_status\":%lu,\"sd_last_block_"       \
    "frames\":%lu,\"sd_write_last_us\":%lu,\"sd_sync_last_us\":%lu,"           \
    "\"sd_store_block_last_us\":%lu,\"sd_write_max_since_status_us\":%lu,"     \
    "\"sd_sync_max_since_status_us\":%lu,\"sd_store_block_max_since_status_"   \
    "us\":%lu,\"sd_store_block_avg_since_status_us\":%lu,"                     \
    "\"sd_sync_skipped_since_status\":%lu,"                                    \
    "\"can1_rx_highwater_pct\":%.2f,\"can2_rx_highwater_pct\":%.2f,"           \
    "\"can1_hw_rx_fifo_highwater_pct\":%.2f,\"can2_hw_rx_fifo_highwater_"      \
    "pct\":%.2f,\"fdcan_msg_port_highwater_pct\":%.2f,"                        \
    "\"can1_psr\":%lu,\"can1_ecr\":%lu,\"can1_rxf0s\":%lu,\"can1_ir\":"        \
    "%lu,\"can1_psr_latched\":%lu,\"can1_ecr_latched\":%lu,"                   \
    "\"can1_rxf0s_latched\":%lu,\"can1_ir_latched\":%lu,"                      \
    "\"can2_psr\":%lu,\"can2_ecr\":%lu,\"can2_rxf0s\":%lu,\"can2_ir\":"        \
    "%lu,\"can2_psr_latched\":%lu,\"can2_ecr_latched\":%lu,"                   \
    "\"can2_rxf0s_latched\":%lu,\"can2_ir_latched\":%lu,"                      \
    "\"sd_write_max_us\":%lu,\"sd_sync_max_us\":%lu,\"sd_store_block_max_"     \
    "us\":"                                                                    \
    "%lu,\"can1_tx_replay_requests\":%lu,\"can1_tx_replay_completed\":%lu,"    \
    "\"can1_tx_replay_irqs\":%lu,\"can1_tx_replay_last_completed_mask\":%lu,"  \
    "\"can1_tx_replay_buffer_mask\":%lu,\"can1_tx_replay_next_id_offset\":"    \
    "%lu,\"can1_tx_replay_txbrp\":%lu,"                                        \
    "\"can1_tx_replay_txbto\":%lu,\"can1_tx_replay_txbcf\":%lu,"               \
    "\"can1_tx_replay_psr\":%lu,\"can1_tx_replay_ecr\":%lu,"                   \
    "\"can1_tx_replay_reload_errors\":%lu,\"can1_tx_replay_active\":%s,"       \
    "\"can2_tx_replay_requests\":%lu,\"can2_tx_replay_completed\":%lu,"        \
    "\"can2_tx_replay_irqs\":%lu,\"can2_tx_replay_last_completed_mask\":%lu,"  \
    "\"can2_tx_replay_buffer_mask\":%lu,\"can2_tx_replay_next_id_offset\":"    \
    "%lu,\"can2_tx_replay_txbrp\":%lu,"                                        \
    "\"can2_tx_replay_txbto\":%lu,\"can2_tx_replay_txbcf\":%lu,"               \
    "\"can2_tx_replay_psr\":%lu,\"can2_tx_replay_ecr\":%lu,"                   \
    "\"can2_tx_replay_reload_errors\":%lu,\"can2_tx_replay_active\":%s}"

#define CANLOG_CONFIG_STRING                                                   \
    "{\"cluster_size\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}"

#define CANLOG_DEBUG_COUNTER_PAIR(name)                                        \
    "\"" name "_hi\":%lu,\"" name "_lo\":%lu"

#define CANLOG_DEBUG_STRING                                                    \
    "{ " CANLOG_DEBUG_COUNTER_PAIR("can_rx_frames"                             \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can_rx_dropped_frames"                    \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_rx_dropped_frames"                   \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_rx_dropped_frames"                   \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("bridge_ingress_frames"                    \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("logger_ingress_frames"                    \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("bridge_logger_backlog_frames"             \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("logger_buffer_rejected_frames"            \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("logger_buffer_full_frames"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_sequence_gap_frames"                 \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_sequence_gap_frames"                 \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_id_sequence_errors"                  \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_id_sequence_errors"                  \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("sd_buffer_ingress_frames"                 \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("sd_buffer_egress_frames"                  \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("sd_buffer_dropped_frames"                 \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("sd_blocks_written"                        \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("message_port_dropped_frames"              \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("runtime_check_error_calls"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_rx_fifo0_lost_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_rx_fifo0_lost_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_protocol_error_events"               \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_protocol_error_events"               \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_error_warning_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_error_warning_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_error_passive_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_error_passive_events"                \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can1_bus_off_events"                      \
    ) "," CANLOG_DEBUG_COUNTER_PAIR("can2_bus_off_events") "}"

#define CANLOG_DEBUG_ERROR_STRING                                              \
    "{\"error_reason\":\"%s\",\"payload_size\":%lu,\"max_size\":%lu}"

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
static char DebugData[JSON_RESPONSE_HEADER_SIZE + CANLOG_MAX_DEBUG_SIZE];
static char ConfigData[CANLOG_MAX_CONFIG_SIZE];
static char IapStatusData[IAP_STATUS_MAX_SIZE];
static char IapPrepareResultData[96U];
static char IapApplyResultData[96U];
static FsCustomDebugProviderType DebugProvider = {0};

uint8_t FsCustom_SetDebugProvider(const FsCustomDebugProviderType *provider)
{
    if ((NULL == provider) || (NULL == provider->read))
    {
        return 1U;
    }

    DebugProvider = *provider;
    return 0U;
}

static bool FsCustom_AppendErrorEntry(
    char *buffer,
    size_t buffer_size,
    size_t *offset,
    const ErrorRecorderEntryType *entry
)
{
    const ErrorContextType *context;
    char file[ERRORCONTEXT_FILE_NAME_LENGTH + 1U];
    char function[ERRORCONTEXT_FUNCTION_NAME_LENGTH + 1U];

    if (NULL == entry)
    {
        return FsCustom_AppendJson(buffer, buffer_size, offset, "null");
    }

    context = &entry->context;
    memcpy(file, context->file, ERRORCONTEXT_FILE_NAME_LENGTH);
    file[ERRORCONTEXT_FILE_NAME_LENGTH] = '\0';
    memcpy(function, context->function, ERRORCONTEXT_FUNCTION_NAME_LENGTH);
    function[ERRORCONTEXT_FUNCTION_NAME_LENGTH] = '\0';

    return FsCustom_AppendJson(
        buffer,
        buffer_size,
        offset,
        "{\"seq\":%lu,\"id\":%lu,\"code\":%lu,\"line\":%u,"
        "\"arg0\":%lu,\"arg1\":%lu,\"file\":\"%s\",\"function\":\"%s\"}",
        (unsigned long)entry->sequence,
        (unsigned long)context->id,
        (unsigned long)context->code,
        (unsigned int)context->line,
        (unsigned long)context->arg0,
        (unsigned long)context->arg1,
        file,
        function
    );
}

static bool FsCustom_AppendErrorEntryOrNull(
    char *buffer,
    size_t buffer_size,
    size_t *offset,
    uint8_t valid,
    const ErrorRecorderEntryType *entry
)
{
    return (0U != valid)
               ? FsCustom_AppendErrorEntry(buffer, buffer_size, offset, entry)
               : FsCustom_AppendJson(buffer, buffer_size, offset, "null");
}

static bool FsCustom_BuildErrorRecorderJson(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    ErrorRecorderSnapshotType Snapshot;
    size_t offset = 0U;

    if ((NULL == buffer) || (NULL == payload_size)
        || (0U != ErrorRecorder_GetSnapshot(&Snapshot)))
    {
        return false;
    }

    if (!FsCustom_AppendJson(
            buffer,
            buffer_size,
            &offset,
            "{\"total\":%lu,\"overwritten\":%lu,\"first\":",
            (unsigned long)Snapshot.total_count,
            (unsigned long)Snapshot.overwritten_count
        ))
    {
        return false;
    }

    if (!FsCustom_AppendErrorEntryOrNull(
            buffer,
            buffer_size,
            &offset,
            Snapshot.first_valid,
            &Snapshot.first
        ))
    {
        return false;
    }

    if (!FsCustom_AppendJson(buffer, buffer_size, &offset, ",\"last\":"))
    {
        return false;
    }

    if (!FsCustom_AppendErrorEntryOrNull(
            buffer,
            buffer_size,
            &offset,
            Snapshot.last_valid,
            &Snapshot.last
        ))
    {
        return false;
    }

    if (!FsCustom_AppendJson(
            buffer,
            buffer_size,
            &offset,
            ",\"recent_count\":%u,\"recent\":[",
            (unsigned int)Snapshot.recent_count
        ))
    {
        return false;
    }

    for (uint8_t i = 0U; i < Snapshot.recent_count; i++)
    {
        if ((i > 0U) && !FsCustom_AppendJson(buffer, buffer_size, &offset, ","))
        {
            return false;
        }

        if (!FsCustom_AppendErrorEntry(
                buffer,
                buffer_size,
                &offset,
                &Snapshot.recent[i]
            ))
        {
            return false;
        }
    }

    if (!FsCustom_AppendJson(buffer, buffer_size, &offset, "]}"))
    {
        return false;
    }

    *payload_size = offset;
    return true;
}

/* accept only files inside /logs/ and beginning with CAN.LOG ---- */
static int FsCustomLogs_OpenFile(struct fs_file *file, const char *name)
{
    uint32_t FileSize                     = 0U;
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

    if (FR_OK != FatFS_SD_GetFileSize(&CanLogReadFileDevice, &FileSize)
        || FileSize > INT_MAX)
    {
        (void)FatFS_SD_CloseFile(&CanLogReadFileDevice);
        return 0;
    }

    CanLogReadFileDevice.readTargetSize = FileSize;

    file->pextension     = &reqState;
    file->data           = NULL;
    file->len            = (int32_t)FileSize; // tell lwip the total file size
    file->index          = 0;
    file->is_custom_file = 1;

    return 1;
}

static bool
FsCustomLogs_RenderMeta(char *buffer, size_t buffer_size, size_t *payload_size)
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
        buffer,
        buffer_size,
        CANLOG_META_DATA_STRING,
        (unsigned long int)HeadIndex,
        (unsigned long int)TailIndex,
        (unsigned long int)Capacity,
        (unsigned long int)((HeadIndex + Capacity - 1) % Capacity),
        (unsigned long int)(appCanLogGetLogFileSize())
    );

    if (DataSize < 0 || (size_t)DataSize >= buffer_size)
    {
        return false; // Error: formatting failed or buffer too small
    }

    *payload_size = (size_t)DataSize;
    return true;
}

static bool FsCustomLogs_RenderConfig(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    int DataSize;

    DataSize = snprintf(
        buffer,
        buffer_size,
        CANLOG_CONFIG_STRING,
        (unsigned long int)(appCanLogGetClusterSize()),
        (unsigned long int)(appCanLogGetLogFileSize()),
        (unsigned long int)(appCanLogGetLogFileCount())
    );

    if (DataSize < 0 || (size_t)DataSize >= buffer_size)
    {
        return false; // Error: formatting failed or buffer too small
    }

    *payload_size = (size_t)DataSize;
    return true;
}

static bool FsCustomLogger_RenderStatus(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    uint8_t IsTracerRunning                           = 1;
    float BusLoadCan1                                 = 0.0;
    float BusLoadCan2                                 = 0.0;
    _Bool AnyFrameLost                                = false;
    uint8_t PreallocErrors                            = 0U;
    float Rb1BytesHighWaterPct                        = 0.0f;
    uint32_t CanAbsRxHighWaterCan1                    = 0U;
    uint32_t CanAbsRxHighWaterCan2                    = 0U;
    uint32_t CanAbsRxCapacity                         = 0U;
    float CanAbsRxHighWaterPctCan1                    = 0.0f;
    float CanAbsRxHighWaterPctCan2                    = 0.0f;
    uint32_t CanAbsHwRxFifoHighWaterCan1              = 0U;
    uint32_t CanAbsHwRxFifoHighWaterCan2              = 0U;
    uint32_t CanAbsHwRxFifoCapacity                   = 0U;
    float CanAbsHwRxFifoHighWaterPctCan1              = 0.0f;
    float CanAbsHwRxFifoHighWaterPctCan2              = 0.0f;
    uint32_t FdcanMsgPortHighWater                    = 0U;
    uint32_t FdcanMsgPortCapacity                     = 0U;
    float FdcanMsgPortHighWaterPct                    = 0.0f;
    uint64_t FrameCount                               = 0U;
    unsigned long FrameCountHi                        = 0UL;
    unsigned long FrameCountLo                        = 0UL;
    uint32_t SdWriteMaxUs                             = 0U;
    uint32_t SdSyncMaxUs                              = 0U;
    uint32_t SdStoreBlockMaxUs                        = 0U;
    FsCustomCanLogBufferTelemetryType BufferTelemetry = {0U};
    FsCustomStaticTxReplayStatsType Can1TxReplayStats = {0U};
    FsCustomStaticTxReplayStatsType Can2TxReplayStats = {0U};
    FsCustomFdcanHealthStatsType Can1FdcanHealth      = {0U};
    FsCustomFdcanHealthStatsType Can2FdcanHealth      = {0U};

    if (0 != FsCustom_IsTracerRunning(&IsTracerRunning))
    {
        IsTracerRunning = 1;
    }

    AnyFrameLost = FsCustom_IsAnyFrameLostFlag();
    (void)FsCustom_GetPreallocErrorFlag(&PreallocErrors);
    FsCustom_GetBusloadCan1(&BusLoadCan1);
    FsCustom_GetBusloadCan2(&BusLoadCan2);
    if (0U != FsCustom_GetCanLogBufferTelemetry(&BufferTelemetry))
    {
        memset(&BufferTelemetry, 0, sizeof(BufferTelemetry));
        BufferTelemetry.rb1_bytes_capacity = LOG_BUFFER_SIZE;
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
    if (0U
        != FsCustom_GetCanAbsHwRxFifoHighWaterCan1(&CanAbsHwRxFifoHighWaterCan1
        ))
    {
        CanAbsHwRxFifoHighWaterCan1 = 0U;
    }
    if (0U
        != FsCustom_GetCanAbsHwRxFifoHighWaterCan2(&CanAbsHwRxFifoHighWaterCan2
        ))
    {
        CanAbsHwRxFifoHighWaterCan2 = 0U;
    }
    if (0U != FsCustom_GetCanAbsHwRxFifoCapacity(&CanAbsHwRxFifoCapacity))
    {
        CanAbsHwRxFifoCapacity = 0U;
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
    (void)FsCustom_GetStaticTxReplayStatsCan1(&Can1TxReplayStats);
    (void)FsCustom_GetStaticTxReplayStatsCan2(&Can2TxReplayStats);
    (void)FsCustom_GetCanAbsFdcanHealthCan1(&Can1FdcanHealth);
    (void)FsCustom_GetCanAbsFdcanHealthCan2(&Can2FdcanHealth);
    FrameCountHi = (unsigned long)((FrameCount >> 32) & 0xFFFFFFFFULL);
    FrameCountLo = (unsigned long)(FrameCount & 0xFFFFFFFFULL);

    if (BufferTelemetry.rb1_bytes_capacity > 0U)
    {
        Rb1BytesHighWaterPct = (float)BufferTelemetry.rb1_bytes_highwater
                               * 100.0f
                               / (float)BufferTelemetry.rb1_bytes_capacity;
    }
    if (CanAbsRxCapacity > 0U)
    {
        CanAbsRxHighWaterPctCan1 =
            (float)CanAbsRxHighWaterCan1 * 100.0f / (float)CanAbsRxCapacity;
        CanAbsRxHighWaterPctCan2 =
            (float)CanAbsRxHighWaterCan2 * 100.0f / (float)CanAbsRxCapacity;
    }
    if (CanAbsHwRxFifoCapacity > 0U)
    {
        CanAbsHwRxFifoHighWaterPctCan1 = (float)CanAbsHwRxFifoHighWaterCan1
                                         * 100.0f
                                         / (float)CanAbsHwRxFifoCapacity;
        CanAbsHwRxFifoHighWaterPctCan2 = (float)CanAbsHwRxFifoHighWaterCan2
                                         * 100.0f
                                         / (float)CanAbsHwRxFifoCapacity;
    }
    if (FdcanMsgPortCapacity > 0U)
    {
        FdcanMsgPortHighWaterPct =
            (float)FdcanMsgPortHighWater * 100.0f / (float)FdcanMsgPortCapacity;
    }

    int n = snprintf(
        buffer,
        buffer_size,
        CANLOG_STATUS_STRING,
        IsTracerRunning ? "true" : "false",
        AnyFrameLost ? "true" : "false",
        PreallocErrors ? "true" : "false",
        FrameCountHi,
        FrameCountLo,
        (double)BusLoadCan1,
        (double)BusLoadCan2,
        (unsigned long)BufferTelemetry.rb1_bytes_capacity,
        (unsigned long)BufferTelemetry.rb1_bytes_now,
        (unsigned long)BufferTelemetry.rb1_bytes_highwater,
        (double)Rb1BytesHighWaterPct,
        (unsigned long)BufferTelemetry.rb1_bytes_min_since_status,
        (unsigned long)BufferTelemetry.rb1_bytes_max_since_status,
        (unsigned long)BufferTelemetry.rb1_bytes_at_last_sd_block_start,
        (unsigned long
        )BufferTelemetry.rb1_bytes_at_last_sd_block_end_before_consume,
        (unsigned long
        )BufferTelemetry.rb1_bytes_at_last_sd_block_end_after_consume,
        (unsigned long)BufferTelemetry.sd_block_attempts_since_status,
        (unsigned long)BufferTelemetry.sd_blocks_written_since_status,
        (unsigned long)BufferTelemetry.sd_block_errors_since_status,
        (unsigned long)BufferTelemetry.sd_last_block_frames,
        (unsigned long)BufferTelemetry.sd_write_last_us,
        (unsigned long)BufferTelemetry.sd_sync_last_us,
        (unsigned long)BufferTelemetry.sd_store_block_last_us,
        (unsigned long)BufferTelemetry.sd_write_max_since_status_us,
        (unsigned long)BufferTelemetry.sd_sync_max_since_status_us,
        (unsigned long)BufferTelemetry.sd_store_block_max_since_status_us,
        (unsigned long)BufferTelemetry.sd_store_block_avg_since_status_us,
        (unsigned long)BufferTelemetry.sd_sync_skipped_since_status,
        (double)CanAbsRxHighWaterPctCan1,
        (double)CanAbsRxHighWaterPctCan2,
        (double)CanAbsHwRxFifoHighWaterPctCan1,
        (double)CanAbsHwRxFifoHighWaterPctCan2,
        (double)FdcanMsgPortHighWaterPct,
        (unsigned long)Can1FdcanHealth.protocol_status,
        (unsigned long)Can1FdcanHealth.error_counter,
        (unsigned long)Can1FdcanHealth.rx_fifo0_status,
        (unsigned long)Can1FdcanHealth.interrupt_flags,
        (unsigned long)Can1FdcanHealth.protocol_status_latched,
        (unsigned long)Can1FdcanHealth.error_counter_latched,
        (unsigned long)Can1FdcanHealth.rx_fifo0_status_latched,
        (unsigned long)Can1FdcanHealth.interrupt_flags_latched,
        (unsigned long)Can2FdcanHealth.protocol_status,
        (unsigned long)Can2FdcanHealth.error_counter,
        (unsigned long)Can2FdcanHealth.rx_fifo0_status,
        (unsigned long)Can2FdcanHealth.interrupt_flags,
        (unsigned long)Can2FdcanHealth.protocol_status_latched,
        (unsigned long)Can2FdcanHealth.error_counter_latched,
        (unsigned long)Can2FdcanHealth.rx_fifo0_status_latched,
        (unsigned long)Can2FdcanHealth.interrupt_flags_latched,
        (unsigned long)SdWriteMaxUs,
        (unsigned long)SdSyncMaxUs,
        (unsigned long)SdStoreBlockMaxUs,
        (unsigned long)Can1TxReplayStats.requests,
        (unsigned long)Can1TxReplayStats.completed,
        (unsigned long)Can1TxReplayStats.irqs,
        (unsigned long)Can1TxReplayStats.last_completed_mask,
        (unsigned long)Can1TxReplayStats.buffer_mask,
        (unsigned long)Can1TxReplayStats.next_id_offset,
        (unsigned long)Can1TxReplayStats.tx_pending,
        (unsigned long)Can1TxReplayStats.tx_occurred,
        (unsigned long)Can1TxReplayStats.tx_cancelled,
        (unsigned long)Can1TxReplayStats.protocol_status,
        (unsigned long)Can1TxReplayStats.error_counter,
        (unsigned long)Can1TxReplayStats.reload_errors,
        Can1TxReplayStats.active ? "true" : "false",
        (unsigned long)Can2TxReplayStats.requests,
        (unsigned long)Can2TxReplayStats.completed,
        (unsigned long)Can2TxReplayStats.irqs,
        (unsigned long)Can2TxReplayStats.last_completed_mask,
        (unsigned long)Can2TxReplayStats.buffer_mask,
        (unsigned long)Can2TxReplayStats.next_id_offset,
        (unsigned long)Can2TxReplayStats.tx_pending,
        (unsigned long)Can2TxReplayStats.tx_occurred,
        (unsigned long)Can2TxReplayStats.tx_cancelled,
        (unsigned long)Can2TxReplayStats.protocol_status,
        (unsigned long)Can2TxReplayStats.error_counter,
        (unsigned long)Can2TxReplayStats.reload_errors,
        Can2TxReplayStats.active ? "true" : "false"
    );

    if (n < 0 || (size_t)n >= buffer_size)
    {
        return false;
    }

    *payload_size = (size_t)n;
    return true;
}

static bool FsCustomLogger_RenderDebug(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    FsCustomDebugSnapshotType Snapshot;
    int n = 0;

    if ((NULL == DebugProvider.read)
        || (0U != DebugProvider.read(DebugProvider.context, &Snapshot)))
    {
        return false;
    }

    n = snprintf(
        buffer,
        buffer_size,
        CANLOG_DEBUG_STRING,
        U64_HI(Snapshot.can_rx_frames),
        U64_LO(Snapshot.can_rx_frames),
        U64_HI(Snapshot.can_rx_dropped_frames),
        U64_LO(Snapshot.can_rx_dropped_frames),
        U64_HI(Snapshot.can1_rx_dropped_frames),
        U64_LO(Snapshot.can1_rx_dropped_frames),
        U64_HI(Snapshot.can2_rx_dropped_frames),
        U64_LO(Snapshot.can2_rx_dropped_frames),
        U64_HI(Snapshot.bridge_ingress_frames),
        U64_LO(Snapshot.bridge_ingress_frames),
        U64_HI(Snapshot.logger_ingress_frames),
        U64_LO(Snapshot.logger_ingress_frames),
        U64_HI(Snapshot.bridge_logger_backlog_frames),
        U64_LO(Snapshot.bridge_logger_backlog_frames),
        U64_HI(Snapshot.logger_buffer_rejected_frames),
        U64_LO(Snapshot.logger_buffer_rejected_frames),
        U64_HI(Snapshot.logger_buffer_full_frames),
        U64_LO(Snapshot.logger_buffer_full_frames),
        U64_HI(Snapshot.can1_sequence_gap_frames),
        U64_LO(Snapshot.can1_sequence_gap_frames),
        U64_HI(Snapshot.can2_sequence_gap_frames),
        U64_LO(Snapshot.can2_sequence_gap_frames),
        U64_HI(Snapshot.can1_id_sequence_errors),
        U64_LO(Snapshot.can1_id_sequence_errors),
        U64_HI(Snapshot.can2_id_sequence_errors),
        U64_LO(Snapshot.can2_id_sequence_errors),
        U64_HI(Snapshot.sd_buffer_ingress_frames),
        U64_LO(Snapshot.sd_buffer_ingress_frames),
        U64_HI(Snapshot.sd_buffer_egress_frames),
        U64_LO(Snapshot.sd_buffer_egress_frames),
        U64_HI(Snapshot.sd_buffer_dropped_frames),
        U64_LO(Snapshot.sd_buffer_dropped_frames),
        U64_HI(Snapshot.sd_blocks_written),
        U64_LO(Snapshot.sd_blocks_written),
        U64_HI(Snapshot.message_port_dropped_frames),
        U64_LO(Snapshot.message_port_dropped_frames),
        U64_HI(Snapshot.runtime_check_error_calls),
        U64_LO(Snapshot.runtime_check_error_calls),
        U64_HI(Snapshot.can1_rx_fifo0_lost_events),
        U64_LO(Snapshot.can1_rx_fifo0_lost_events),
        U64_HI(Snapshot.can2_rx_fifo0_lost_events),
        U64_LO(Snapshot.can2_rx_fifo0_lost_events),
        U64_HI(Snapshot.can1_protocol_error_events),
        U64_LO(Snapshot.can1_protocol_error_events),
        U64_HI(Snapshot.can2_protocol_error_events),
        U64_LO(Snapshot.can2_protocol_error_events),
        U64_HI(Snapshot.can1_error_warning_events),
        U64_LO(Snapshot.can1_error_warning_events),
        U64_HI(Snapshot.can2_error_warning_events),
        U64_LO(Snapshot.can2_error_warning_events),
        U64_HI(Snapshot.can1_error_passive_events),
        U64_LO(Snapshot.can1_error_passive_events),
        U64_HI(Snapshot.can2_error_passive_events),
        U64_LO(Snapshot.can2_error_passive_events),
        U64_HI(Snapshot.can1_bus_off_events),
        U64_LO(Snapshot.can1_bus_off_events),
        U64_HI(Snapshot.can2_bus_off_events),
        U64_LO(Snapshot.can2_bus_off_events)
    );

    if (n < 0 || (size_t)n >= buffer_size)
    {
        unsigned long PayloadSize = 0UL;
        const char *Reason        = "debug_payload_format_failed";

        if (n >= 0)
        {
            PayloadSize = (unsigned long)n;
            Reason      = "debug_payload_too_large";
        }

        n = snprintf(
            buffer,
            buffer_size,
            CANLOG_DEBUG_ERROR_STRING,
            Reason,
            PayloadSize,
            (unsigned long)buffer_size
        );
        if (n < 0 || (size_t)n >= buffer_size)
        {
            return false;
        }
    }

    *payload_size = (size_t)n;
    return true;
}

static bool FsCustomLogger_RenderErrors(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    size_t ErrorPayloadSize = 0U;
    int ErrorDataSize;

    if (FsCustom_BuildErrorRecorderJson(buffer, buffer_size, &ErrorPayloadSize))
    {
        *payload_size = ErrorPayloadSize;
        return true;
    }

    ErrorDataSize = snprintf(
        buffer,
        buffer_size,
        CANLOG_DEBUG_ERROR_STRING,
        "error_recorder_payload_build_failed",
        (unsigned long)ErrorPayloadSize,
        (unsigned long)buffer_size
    );

    if ((ErrorDataSize < 0) || ((size_t)ErrorDataSize >= buffer_size))
    {
        return false;
    }

    *payload_size = (size_t)ErrorDataSize;
    return true;
}

static int FsCustom_OpenPostRedirect(struct fs_file *file, const char *name)
{
    (void)name;

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

typedef int (*FsCustomOpenFn)(struct fs_file *file, const char *name);
typedef bool (*FsCustomRenderFn)(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
);

typedef struct
{
    const char *path;
    size_t path_length;
    FsCustomOpenFn open; /* stream / const data; NULL for rendered routes */
    FsCustomRenderFn render; /* fills buffer; used when open == NULL */
    char *buffer;
    size_t buffer_size;
    uint8_t flags; /* FS_FILE_FLAGS_HEADER_INCLUDED: JSON header is prepended */
} FsCustomRouteType;

#define FS_CUSTOM_ROUTE(path, handler)                                         \
    {                                                                          \
        (path), sizeof(path) - 1U, (handler), NULL, NULL, 0U, 0U               \
    }
#define FS_CUSTOM_RENDER_ROUTE(path, render, buffer, flags)                    \
    {                                                                          \
        (path), sizeof(path) - 1U, NULL, (render), (buffer), sizeof(buffer),   \
            (flags)                                                            \
    }

/* Prefix match, first hit wins: keep the order. */
static const FsCustomRouteType FsCustomRoutes[] = {
    FS_CUSTOM_ROUTE(CANLOG_FILE_PATH, FsCustomLogs_OpenFile),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_META_DATA_PATH,
        FsCustomLogs_RenderMeta,
        MetaData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_CONFIG_PATH,
        FsCustomLogs_RenderConfig,
        ConfigData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_STATUS_PATH,
        FsCustomLogger_RenderStatus,
        StatusData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_DEBUG_PATH,
        FsCustomLogger_RenderDebug,
        DebugData,
        FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT
    ),
    FS_CUSTOM_RENDER_ROUTE(
        ERRORRECORDER_PATH,
        FsCustomLogger_RenderErrors,
        DebugData,
        FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_STATUS_PATH,
        FsCustomIap_RenderStatus,
        IapStatusData,
        0U
    ),
    FS_CUSTOM_ROUTE(APP_VERSION_PATH, FsCustomApi_OpenVersion),
    FS_CUSTOM_RENDER_ROUTE(
        APP_CARD_PATH,
        FsCustomApi_RenderCard,
        StatusData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_PREPARE_PATH,
        FsCustomIap_RenderPrepare,
        IapPrepareResultData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_APPLY_PATH,
        FsCustomIap_RenderApply,
        IapApplyResultData,
        0U
    ),
    FS_CUSTOM_ROUTE(CANLOG_POST_REDIRECT_PATH, FsCustom_OpenPostRedirect),
};

/* Render into the route's buffer and publish it. For routes with
 * FS_FILE_FLAGS_HEADER_INCLUDED the JSON response header goes first and the
 * payload follows it. */
static int
FsCustom_OpenRendered(struct fs_file *file, const FsCustomRouteType *route)
{
    size_t HeaderSize  = 0U;
    size_t PayloadSize = 0U;

    if (0U != (route->flags & FS_FILE_FLAGS_HEADER_INCLUDED))
    {
        HeaderSize = JSON_RESPONSE_HEADER_SIZE;
    }

    if ((route->buffer_size <= HeaderSize)
        || !route->render(
            &route->buffer[HeaderSize],
            route->buffer_size - HeaderSize,
            &PayloadSize
        ))
    {
        return 0;
    }

    memcpy(route->buffer, JSON_RESPONSE_HEADER, HeaderSize);

    file->data           = route->buffer;
    file->len            = (int)(HeaderSize + PayloadSize);
    file->index          = 0;
    file->flags          = route->flags;
    file->is_custom_file = 0; /* httpd sends static buffer */
    return 1;
}

int fs_open_custom(struct fs_file *file, const char *name)
{
    memset(file, 0, sizeof(*file));

    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(FsCustomRoutes); i++)
    {
        const FsCustomRouteType *route = &FsCustomRoutes[i];

        if (0 == strncmp(name, route->path, route->path_length))
        {
            return (NULL != route->open) ? route->open(file, name)
                                         : FsCustom_OpenRendered(file, route);
        }
    }

    return 0; // Fallback to default file system
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
        len = (uint32_t)count;
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

    return (int)len; // triggers send
}

void fs_close_custom(struct fs_file *file)
{
    FatFS_SD_CloseFile(&CanLogReadFileDevice);
    file->pextension = NULL; // optional cleanup
}

#endif
