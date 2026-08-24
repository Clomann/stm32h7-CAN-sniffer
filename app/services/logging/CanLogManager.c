#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "CanLogManager.h"

#include "CanLogManagerTypes.h"
#include "ErrorContext.h"
#include "SettingsHandler.h"
#include "ff.h"
#include "fs_custom.h"
#include "CanLogBuffer.h"
#include "CanAbs.h"
#include "CanCtrl.h"
#include "fdcan_msg_port.h"
#include "SdBridgeTask.h"
#include "core_json.h"
#include "RuntimeChecks.h"
#include "SequenceChecker.h"

#include "instrumentation.h"
#if INSTR_ENABLED
#include "CommTypes.h"
#include "FileHandler.h"
#endif

#if !defined(UNIT_TEST)
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#ifndef DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
/* Integration-test aid for generated CAN traffic.
 * Static replay builds check one complete ID set per replay batch. */
#define DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE 0U
#endif

#ifndef DEBUG_CAN_ID_SEQUENCE_MAX_ID
#define DEBUG_CAN_ID_SEQUENCE_MAX_ID 0xFFU
#endif

#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE && CAN_STATIC_TX_REPLAY_ENABLE           \
    && (CAN_STATIC_TX_REPLAY_ID_COUNT < CAN_STATIC_TX_REPLAY_TX_BUFFERS)
#error                                                                         \
    "DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE requires one unique static replay ID per Tx buffer"
#endif

#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE && CAN_STATIC_TX_REPLAY_ENABLE
typedef struct
{
    uint32_t base_id;
    uint32_t id_count;
} CanLogStaticReplayIdCheckType;

static void CanLogManager_StaticReplayIdCheckInit(
    CanLogStaticReplayIdCheckType *check,
    uint32_t base_id,
    uint32_t id_count
)
{
    check->base_id  = base_id & 0x7FFU;
    check->id_count = (0U == id_count) ? 1U : id_count;
}

static bool CanLogManager_StaticReplayIdCheck(
    CanLogStaticReplayIdCheckType *check,
    uint32_t can_id
)
{
    uint32_t offset;
    uint32_t bit;

    offset = ((can_id & 0x7FFU) + 0x800U - check->base_id) & 0x7FFU;
    if (offset >= CAN_STATIC_TX_REPLAY_TX_BUFFERS)
    {
        return false;
    }

    bit = 1UL << offset;
    if ((check->seen_mask & bit) != 0U)
    {
        return false;
    }

    check->seen_mask |= bit;
    if (check->seen_mask == check->expected_mask)
    {
        check->seen_mask = 0U;
    }

    return true;
}
#endif

#define CANLOG_INGEST_BATCH_MAX 256

#define CAN_LOG_PREALLOC_MARKER_FILENAME "/logs/prealloc.json"
#define CAN_LOG_PREALLOC_MARKER_VERSION  1U
#define CAN_LOG_PREALLOC_MARKER_MAX_SIZE 128U

#define CLM_ABS_TIME_TO_TIMSTAMP(x) (uint32_t)(x)
#define CLM_ABS_TIME_TO_ABS_HIGH(x) ((uint32_t)((x) >> 32U))
#define CLM_SYNC_EMIT_INTERVAL_US   (30ULL * 60ULL * 1000000ULL)

extern uint64_t FDCAN_GetTimestampHook(void);

void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);
void CanLogManager_DrainPortStartHook(void);
void CanLogManager_DrainPortEndHook(void);
void CanLogManager_InstrumentationFdcanMsgPortPeakHook(
    uint32_t timestamp_us,
    uint32_t used_bytes
);

typedef struct
{
    uint32_t epoch;
    uint32_t fileIndex;
    uint32_t byteOffset;
    uint32_t crc;
} CanLogMetaDataType;

static volatile CanLogMetaDataType LogMetaData;
#if CANLOGAMANGER_PERSIST_METADATA
static uint8_t LogMetaDataOpenRes;
#endif

typedef struct
{
    uint64_t receivedFrames;
    uint32_t prevTimestamp;
    uint32_t baudrate;
    float busLoad;
    uint32_t accumulatedBits;
    uint32_t accumulatedTime;
} CanStatusDataType;

struct CanLogControlDataType
{
    struct
    {
        char *filename;
        uint32_t fnamemaxlen;
        uint32_t count;
        uint32_t timestamp;
        uint8_t openRes;
        uint32_t fileHeadIndex;
        uint32_t fileTailIndex;
        bool fileIndexWrapped;
        FatFsDeviceType writeFileDevice;
    } CanLog;
    CanStatusDataType Can1;
    CanStatusDataType Can2;
    uint8_t *mountRes;
    bool *runCanTracer;
    bool *commitLog;
    bool runCanTracerOld;
    bool framesLost;
    bool emitSyncEntry;
};

/**
 * @brief Hook called before flushing a CAN log block to storage.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_InstrumentationFlushStartHook(void)
{
}

/**
 * @brief Hook called after flushing a CAN log block to storage.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_InstrumentationFlushEndHook(void)
{
}

/**
 * @brief  Hook called before reading all available frames in the fdcan port buffer.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_DrainPortStartHook(void)
{
}

/**
 * @brief Hook called after reading all available frames in the fdcan port buffer.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_DrainPortEndHook(void)
{
}

volatile static char CanLogFileName[255] = "/logs/CAN.LOG";
volatile static CanLogControlDataType CanLogCtrlData;
static ScSequenceType CAN1_Sequence;
static ScSequenceType CAN2_Sequence;
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if CAN_STATIC_TX_REPLAY_ENABLE
static CanLogStaticReplayIdCheckType CAN1_IdReplayCheck;
static CanLogStaticReplayIdCheckType CAN2_IdReplayCheck;
#else
static ScSequenceType CAN1_IdSequence;
static ScSequenceType CAN2_IdSequence;
#endif
#endif
static uint32_t Rb1BytesHighWater                     = 0U;
static uint32_t CanLogFileSize                        = MAX_LOG_FILE_SIZE;
static uint32_t CanLogFileCount                       = MAX_LOG_FILE_COUNT;
static uint32_t CanLogClusterSize                     = CLUSTER_SIZE;
static volatile uint32_t CanLogPreallocErrorCount     = 0U;
static volatile uint32_t FdcanMsgPortPeakCaptureCount = 0U;
static volatile uint32_t FdcanMsgPortPeakTimestampUs  = 0U;
static volatile uint32_t FdcanMsgPortPeakUsedBytes    = 0U;
static volatile uint32_t FdcanMsgPortPeakCanLogBufferUsedBytes   = 0U;
static volatile uint32_t FdcanMsgPortPeakRb1HighWaterBytes       = 0U;
static volatile uint32_t FdcanMsgPortPeakFileHeadIndex           = 0U;
static volatile uint32_t FdcanMsgPortPeakMetaFileIndex           = 0U;
static volatile uint32_t FdcanMsgPortPeakMetaByteOffset          = 0U;
static volatile uint64_t FdcanMsgPortPeakCanLogManagerFrameCount = 0U;
static volatile uint64_t FdcanMsgPortPeakCanLogBufferBlockCount  = 0U;
static volatile uint32_t CanLogSdWriteMaxUs                      = 0U;
static volatile uint32_t CanLogSdSyncMaxUs                       = 0U;
static volatile uint32_t CanLogSdStoreBlockMaxUs                 = 0U;
static volatile uint8_t Rb1TelemetryWindowInitialized            = 0U;
static volatile uint32_t Rb1BytesMinSinceStatus                  = 0U;
static volatile uint32_t Rb1BytesMaxSinceStatus                  = 0U;
static volatile uint32_t Rb1BytesAtLastSdBlockStart              = 0U;
static volatile uint32_t Rb1BytesAtLastSdBlockEndBeforeConsume   = 0U;
static volatile uint32_t Rb1BytesAtLastSdBlockEndAfterConsume    = 0U;
static volatile uint32_t CanLogSdBlockAttemptsSinceStatus        = 0U;
static volatile uint32_t CanLogSdBlocksWrittenSinceStatus        = 0U;
static volatile uint32_t CanLogSdBlockErrorsSinceStatus          = 0U;
static volatile uint32_t CanLogSdLastBlockFrames                 = 0U;
static volatile uint32_t CanLogSdWriteLastUs                     = 0U;
static volatile uint32_t CanLogSdSyncLastUs                      = 0U;
static volatile uint32_t CanLogSdStoreBlockLastUs                = 0U;
static volatile uint32_t CanLogSdWriteMaxSinceStatusUs           = 0U;
static volatile uint32_t CanLogSdSyncMaxSinceStatusUs            = 0U;
static volatile uint32_t CanLogSdStoreBlockMaxSinceStatusUs      = 0U;
static volatile uint64_t CanLogSdStoreBlockSumSinceStatusUs      = 0U;

static uint32_t CanLogManager_ReadRb1UsedBytes(void)
{
    uint32_t used = 0U;

    (void)CanLogBuffer_UsedBytes(&used);
    return used;
}

static void CanLogManager_RecordRb1UsedBytes(uint32_t used)
{
    if (0U == Rb1TelemetryWindowInitialized)
    {
        Rb1BytesMinSinceStatus        = used;
        Rb1BytesMaxSinceStatus        = used;
        Rb1TelemetryWindowInitialized = 1U;
    }
    else
    {
        if (used < Rb1BytesMinSinceStatus)
        {
            Rb1BytesMinSinceStatus = used;
        }
        if (used > Rb1BytesMaxSinceStatus)
        {
            Rb1BytesMaxSinceStatus = used;
        }
    }
}

static void CanLogManager_ResetBufferTelemetryWindow(uint32_t rb1_used)
{
    Rb1TelemetryWindowInitialized      = 1U;
    Rb1BytesMinSinceStatus             = rb1_used;
    Rb1BytesMaxSinceStatus             = rb1_used;
    CanLogSdBlockAttemptsSinceStatus   = 0U;
    CanLogSdBlocksWrittenSinceStatus   = 0U;
    CanLogSdBlockErrorsSinceStatus     = 0U;
    CanLogSdWriteMaxSinceStatusUs      = 0U;
    CanLogSdSyncMaxSinceStatusUs       = 0U;
    CanLogSdStoreBlockMaxSinceStatusUs = 0U;
    CanLogSdStoreBlockSumSinceStatusUs = 0U;
}

static void CanLogManager_ResetBufferTelemetry(uint32_t rb1_used)
{
    Rb1BytesHighWater                     = 0U;
    Rb1BytesAtLastSdBlockStart            = 0U;
    Rb1BytesAtLastSdBlockEndBeforeConsume = 0U;
    Rb1BytesAtLastSdBlockEndAfterConsume  = 0U;
    CanLogSdBlockAttemptsSinceStatus      = 0U;
    CanLogSdBlocksWrittenSinceStatus      = 0U;
    CanLogSdBlockErrorsSinceStatus        = 0U;
    CanLogSdLastBlockFrames               = 0U;
    CanLogSdWriteLastUs                   = 0U;
    CanLogSdSyncLastUs                    = 0U;
    CanLogSdStoreBlockLastUs              = 0U;
    CanLogSdWriteMaxSinceStatusUs         = 0U;
    CanLogSdSyncMaxSinceStatusUs          = 0U;
    CanLogSdStoreBlockMaxSinceStatusUs    = 0U;
    CanLogSdStoreBlockSumSinceStatusUs    = 0U;

    CanLogManager_ResetBufferTelemetryWindow(rb1_used);
}

static void CanLogManager_RecordSdBlockTiming(
    uint32_t write_us,
    uint32_t sync_us,
    uint32_t store_block_us,
    uint32_t frame_count,
    comm_status_t result
)
{
    CanLogSdBlockAttemptsSinceStatus++;
    CanLogSdLastBlockFrames  = frame_count;
    CanLogSdWriteLastUs      = write_us;
    CanLogSdSyncLastUs       = sync_us;
    CanLogSdStoreBlockLastUs = store_block_us;

    if (write_us > CanLogSdWriteMaxSinceStatusUs)
    {
        CanLogSdWriteMaxSinceStatusUs = write_us;
    }

    if (COMM_SUCCESS == result)
    {
        CanLogSdBlocksWrittenSinceStatus++;
        CanLogSdStoreBlockSumSinceStatusUs += store_block_us;

        if (sync_us > CanLogSdSyncMaxSinceStatusUs)
        {
            CanLogSdSyncMaxSinceStatusUs = sync_us;
        }
        if (store_block_us > CanLogSdStoreBlockMaxSinceStatusUs)
        {
            CanLogSdStoreBlockMaxSinceStatusUs = store_block_us;
        }
    }
    else
    {
        CanLogSdBlockErrorsSinceStatus++;
    }
}

#if !defined(UNIT_TEST)
static StaticSemaphore_t CanLogFileMutexBuffer;
static SemaphoreHandle_t CanLogFileMutex = NULL;
#endif

void CanLogManager_InstrumentationFdcanMsgPortPeakHook(
    uint32_t timestamp_us,
    uint32_t used_bytes
)
{
    uint32_t rb1_used_bytes = 0U;

    (void)CanLogBuffer_UsedBytes(&rb1_used_bytes);

    FdcanMsgPortPeakCaptureCount++;
    FdcanMsgPortPeakTimestampUs           = timestamp_us;
    FdcanMsgPortPeakUsedBytes             = used_bytes;
    FdcanMsgPortPeakCanLogBufferUsedBytes = rb1_used_bytes;
    FdcanMsgPortPeakRb1HighWaterBytes     = Rb1BytesHighWater;
    FdcanMsgPortPeakFileHeadIndex         = CanLogCtrlData.CanLog.fileHeadIndex;
    FdcanMsgPortPeakMetaFileIndex         = LogMetaData.fileIndex;
    FdcanMsgPortPeakMetaByteOffset        = LogMetaData.byteOffset;
    FdcanMsgPortPeakCanLogManagerFrameCount = CanLogManager_FrameCount;
    FdcanMsgPortPeakCanLogBufferBlockCount  = CanLogBuffer_BlockCount;
}

static void CanLogManager_FileLockInit(void)
{
#if !defined(UNIT_TEST)
    if (CanLogFileMutex == NULL)
    {
        CanLogFileMutex = xSemaphoreCreateMutexStatic(&CanLogFileMutexBuffer);
        configASSERT(CanLogFileMutex != NULL);
    }
#endif
}

static void CanLogManager_FileLock(void)
{
#if !defined(UNIT_TEST)
    if (CanLogFileMutex == NULL)
    {
        CanLogManager_FileLockInit();
    }

    if (CanLogFileMutex != NULL)
    {
        (void)xSemaphoreTake(CanLogFileMutex, portMAX_DELAY);
    }
#endif
}

static void CanLogManager_FileUnlock(void)
{
#if !defined(UNIT_TEST)
    if (CanLogFileMutex != NULL)
    {
        (void)xSemaphoreGive(CanLogFileMutex);
    }
#endif
}

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix);
static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data);
static unsigned int appCanLogCheckNewFileOpenLocked(void);
static void appCanLogFillEntry(
    CanLogEntryType *entry,
    FDCAN_ClassicFrameType *frame,
    uint64_t timestamp,
    uint8_t channel
);
static comm_status_t appCanLogStoreToFrameBuffer(void *entry);
static comm_status_t appCanLogStoreToSd(
    FatFsDeviceType *dev,
    char *data,
    uint32_t length,
    uint32_t *write_us,
    uint32_t *sync_us,
    uint32_t *store_block_us
);
static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev);

static void CanLogManager_UpdateRb1BytesHighWater(void)
{
    uint32_t used = 0U;

    if (CLB_E_OK == CanLogBuffer_UsedBytes(&used))
    {
        CanLogManager_RecordRb1UsedBytes(used);

        if (used > Rb1BytesHighWater)
        {
            Rb1BytesHighWater = used;
        }
    }
}

void __attribute__((weak)) CanLogFileManager_ErrorHandler()
{
    __asm volatile("nop");
}

void appCanLogSetFileConfig(uint32_t log_file_size, uint32_t log_file_count)
{
    if (log_file_size == 0U)
    {
        log_file_size = MAX_LOG_FILE_SIZE;
    }

    if (log_file_count == 0U)
    {
        log_file_count = MAX_LOG_FILE_COUNT;
    }

    CanLogFileSize  = log_file_size;
    CanLogFileCount = log_file_count;
}

void appCanLogSetClusterSize(uint32_t cluster_size)
{
    if (cluster_size == 0U)
    {
        cluster_size = CLUSTER_SIZE;
    }

    CanLogClusterSize = cluster_size;
}

uint32_t appCanLogGetLogFileSize(void)
{
    return CanLogFileSize;
}

uint32_t appCanLogGetLogFileCount(void)
{
    return CanLogFileCount;
}

uint32_t appCanLogGetClusterSize(void)
{
    return CanLogClusterSize;
}

uint32_t appCanLogGetMaxLogIndex(void)
{
    if (CanLogFileCount > 0U)
    {
        return CanLogFileCount - 1U;
    }

    return 0U;
}

uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index)
{
    *index = CanLogCtrlData.CanLog.fileHeadIndex;
    return 0U;
}

_Bool FsCustom_IsAnyFrameLostFlag(void)
{
    return CanLogCtrlData.framesLost;
}

uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index)
{
    *index = CanLogCtrlData.CanLog.fileTailIndex;
    return 0U;
}

uint8_t FsCustom_GetCanLogCapacity(uint32_t *capacity)
{
    *capacity = appCanLogGetLogFileCount();
    return 0U;
}

uint8_t FsCustom_IsTracerRunning(uint8_t *running)
{
    uint8_t res;

    res      = 0;
    *running = *CanLogCtrlData.runCanTracer;

    return res;
}

uint8_t FsCustom_GetBusloadCan1(float *busload)
{
    *busload = CanLogCtrlData.Can1.busLoad;
    return 0U;
}

uint8_t FsCustom_GetBusloadCan2(float *busload)
{
    *busload = CanLogCtrlData.Can2.busLoad;
    return 0U;
}

uint8_t FsCustom_GetRb1BytesHighWater(uint32_t *bytes)
{
    *bytes = Rb1BytesHighWater;
    return 0U;
}

uint8_t
FsCustom_GetCanLogBufferTelemetry(FsCustomCanLogBufferTelemetryType *telemetry)
{
    uint32_t rb1_used_now;
    uint32_t sd_block_attempts;
    uint32_t sd_blocks_written;

    if (NULL == telemetry)
    {
        return 1U;
    }

    rb1_used_now = CanLogManager_ReadRb1UsedBytes();
    CanLogManager_RecordRb1UsedBytes(rb1_used_now);

    sd_block_attempts = CanLogSdBlockAttemptsSinceStatus;
    sd_blocks_written = CanLogSdBlocksWrittenSinceStatus;

    telemetry->rb1_bytes_capacity               = LOG_BUFFER_SIZE;
    telemetry->rb1_bytes_now                    = rb1_used_now;
    telemetry->rb1_bytes_highwater              = Rb1BytesHighWater;
    telemetry->rb1_bytes_min_since_status       = Rb1BytesMinSinceStatus;
    telemetry->rb1_bytes_max_since_status       = Rb1BytesMaxSinceStatus;
    telemetry->rb1_bytes_at_last_sd_block_start = Rb1BytesAtLastSdBlockStart;
    telemetry->rb1_bytes_at_last_sd_block_end_before_consume =
        Rb1BytesAtLastSdBlockEndBeforeConsume;
    telemetry->rb1_bytes_at_last_sd_block_end_after_consume =
        Rb1BytesAtLastSdBlockEndAfterConsume;
    telemetry->sd_block_attempts_since_status = sd_block_attempts;
    telemetry->sd_blocks_written_since_status = sd_blocks_written;
    telemetry->sd_block_errors_since_status   = CanLogSdBlockErrorsSinceStatus;
    telemetry->sd_last_block_frames           = CanLogSdLastBlockFrames;
    telemetry->sd_write_last_us               = CanLogSdWriteLastUs;
    telemetry->sd_sync_last_us                = CanLogSdSyncLastUs;
    telemetry->sd_store_block_last_us         = CanLogSdStoreBlockLastUs;
    telemetry->sd_write_max_since_status_us   = CanLogSdWriteMaxSinceStatusUs;
    telemetry->sd_sync_max_since_status_us    = CanLogSdSyncMaxSinceStatusUs;
    telemetry->sd_store_block_max_since_status_us =
        CanLogSdStoreBlockMaxSinceStatusUs;
    telemetry->sd_store_block_avg_since_status_us =
        (sd_blocks_written > 0U)
            ? (uint32_t)(CanLogSdStoreBlockSumSinceStatusUs / sd_blocks_written)
            : 0U;

    CanLogManager_ResetBufferTelemetryWindow(rb1_used_now);

    return 0U;
}

uint8_t FsCustom_GetCanAbsRxHighWaterCan1(uint32_t *frames)
{
    return CanAbs_GetRxHighWater_Can1(frames);
}

uint8_t FsCustom_GetCanAbsRxHighWaterCan2(uint32_t *frames)
{
    return CanAbs_GetRxHighWater_Can2(frames);
}

uint8_t FsCustom_GetCanAbsRxCapacity(uint32_t *frames)
{
    if (frames == NULL)
    {
        return 1U;
    }

    *frames = CanAbs_GetRxBufferCapacity();
    return 0U;
}

uint8_t FsCustom_GetStaticTxReplayStatsCan1(
    uint32_t *requests,
    uint32_t *completed,
    uint32_t *irqs
)
{
    FdcanStaticTxReplayStatsType stats;

    if ((requests == NULL) || (completed == NULL) || (irqs == NULL))
    {
        return 1U;
    }

    if (0U != CanAbs_GetStaticTxReplayStats_Can1(&stats))
    {
        return 1U;
    }

    *requests  = stats.requests;
    *completed = stats.completed;
    *irqs      = stats.irqs;
    return 0U;
}

uint8_t FsCustom_GetStaticTxReplayStatsCan2(
    uint32_t *requests,
    uint32_t *completed,
    uint32_t *irqs
)
{
    FdcanStaticTxReplayStatsType stats;

    if ((requests == NULL) || (completed == NULL) || (irqs == NULL))
    {
        return 1U;
    }

    if (0U != CanAbs_GetStaticTxReplayStats_Can2(&stats))
    {
        return 1U;
    }

    *requests  = stats.requests;
    *completed = stats.completed;
    *irqs      = stats.irqs;
    return 0U;
}

uint8_t FsCustom_GetFdcanMsgPortHighWater(uint32_t *bytes)
{
    return fdcan_msg_port_get_highwater_bytes(bytes);
}

uint8_t FsCustom_GetFdcanMsgPortCapacity(uint32_t *bytes)
{
    if (bytes == NULL)
    {
        return 1U;
    }

    *bytes = fdcan_msg_port_get_capacity_bytes();
    return 0U;
}

uint8_t FsCustom_GetCanLogFrameCount(uint64_t *count)
{
    *count = CanLogManager_FrameCount;
    return 0U;
}

uint8_t FsCustom_GetPreallocErrorFlag(uint8_t *flag)
{
    *flag = (CanLogPreallocErrorCount > 0U) ? 1U : 0U;
    return 0U;
}

uint8_t FsCustom_GetCanLogSdTimingMaxUs(
    uint32_t *write_us,
    uint32_t *sync_us,
    uint32_t *store_block_us
)
{
    if ((NULL == write_us) || (NULL == sync_us) || (NULL == store_block_us))
    {
        return 1U;
    }

    *write_us       = CanLogSdWriteMaxUs;
    *sync_us        = CanLogSdSyncMaxUs;
    *store_block_us = CanLogSdStoreBlockMaxUs;

    return 0U;
}

static bool appCanLogIsValidBaudrate(uint32_t value)
{
    return (value == 250000U) || (value == 500000U) || (value == 1000000U);
}

CanLogResult appCanLogSetParam(ClmParameterIdType id, uint32_t value)
{
    CanLogResult res = CAN_LOG_OK;

    switch (id)
    {
    case CLM_PARAMETER_ID_CAN1_BAUDRATE:
        if (!appCanLogIsValidBaudrate(value))
        {
            res = CAN_LOG_ERR_INVALID_PARAM;
            break;
        }
        CanLogCtrlData.Can1.baudrate = value;
        break;
    case CLM_PARAMETER_ID_CAN2_BAUDRATE:
        if (!appCanLogIsValidBaudrate(value))
        {
            res = CAN_LOG_ERR_INVALID_PARAM;
            break;
        }
        CanLogCtrlData.Can2.baudrate = value;
        break;
    default:
        res = CAN_LOG_ERR_INVALID_PARAM;
        break;
    }

    return res;
}

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix)
{
    FatFS_FileIterator it;
    FILINFO *fno;
    int highest = -1;

    if (FatFS_SD_FileIterator_Open(&it, dirPath, prefix) != FR_OK)
    {
        return -1;
    }

    while (FatFS_SD_FileIterator_Next(&it, &fno) == FR_OK)
    {
        const char *suffix = fno->fname + strlen(prefix);
        char *endptr;
        long val = strtol(suffix, &endptr, 10);

        if (*endptr == '\0' && val >= 0 && val <= maxSuffix && val > highest)
        {
            highest = (int)val;
        }
    }

    FatFS_SD_FileIterator_Close(&it);
    return highest;
}

static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data)
{
    int lastUsed;
    uint32_t max_index;
    volatile FRESULT res;

    (void)data;

    max_index = appCanLogGetMaxLogIndex();
    lastUsed  = find_highest_suffix("/logs/", "CAN.LOG", (int)max_index);

#if 0U == PERSIST_CAN_LOG_FILE_HEAD_TAIL
    lastUsed = 0U;
#endif

    CanLogCtrlData.CanLog.fileHeadIndex = lastUsed;

    // Build candidate filename
    snprintf(
        CanLogCtrlData.CanLog.filename,
        CanLogCtrlData.CanLog.fnamemaxlen,
        FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
        (int)lastUsed
    );

    CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        CanLogCtrlData.CanLog.filename
    );

    res = f_lseek(&(CanLogCtrlData.CanLog.writeFileDevice.file), 0);

    return res;
}

#if CANLOGMANAGER_REOPEN_LOG_FILE

static CanLogResult appCanLogReopenFile(CanLogControlDataType *data)
{
    CanLogResult res;

    res = CAN_LOG_OK;

    if (FR_OK
        != FatFS_SD_OpenFileForWrite(
            &(CanLogCtrlData.CanLog.writeFileDevice),
            data->CanLog.filename
        ))
    {
        res = CAN_LOG_NOT_OK;
        CanLogFileManager_ErrorHandler();
    }

    return res;
}

static CanLogResult appCanLogCloseFile(CanLogControlDataType *data)
{
    CanLogResult res;

    res = CAN_LOG_OK;

    if (FR_OK != FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice)))
    {
        res = CAN_LOG_NOT_OK;
        CanLogFileManager_ErrorHandler();
    }

    return res;
}

#endif

static unsigned int appCanLogCheckNewFileOpenLocked(void)
{
    FRESULT FileSizeRes;
    uint32_t FileSize;
    uint32_t log_file_size;
    uint32_t log_file_count;
    uint32_t max_index;

    FileSizeRes = FatFS_SD_GetBufferedFileSize(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        &FileSize
    );

    log_file_size  = appCanLogGetLogFileSize();
    log_file_count = appCanLogGetLogFileCount();
    max_index      = appCanLogGetMaxLogIndex();

    if (FileSizeRes == FR_OK && (FileSize >= log_file_size))
    {
        // File exists and is full, advance to next one
        if (0 == FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice))
            && log_file_count > 0U)
        {
            if (CanLogCtrlData.CanLog.fileHeadIndex >= max_index)
            {
                CanLogCtrlData.CanLog.fileIndexWrapped = 1;
            }

            CanLogCtrlData.CanLog.fileHeadIndex =
                (CanLogCtrlData.CanLog.fileHeadIndex + 1U) % log_file_count;

            if (1 == CanLogCtrlData.CanLog.fileIndexWrapped)
            {
                CanLogCtrlData.CanLog.fileTailIndex =
                    (CanLogCtrlData.CanLog.fileHeadIndex + 1) % log_file_count;
            }

            snprintf(
                CanLogCtrlData.CanLog.filename,
                CanLogCtrlData.CanLog.fnamemaxlen,
                "/logs/CAN.LOG%d",
                (int)CanLogCtrlData.CanLog.fileHeadIndex
            );

#if !PREALLOCATE_LOG_FILES
            (void)f_unlink(CanLogCtrlData.CanLog.filename);
#endif

            CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForOverWrite(
                &(CanLogCtrlData.CanLog.writeFileDevice),
                CanLogCtrlData.CanLog.filename
            );

            if (0 != CanLogCtrlData.CanLog.openRes)
            {
                CanLogFileManager_ErrorHandler();
            }

            if (FR_OK
                != FatFS_SD_Flush(&(CanLogCtrlData.CanLog.writeFileDevice)))
            {
                CanLogFileManager_ErrorHandler();
            }
        }
        else
        {
            CanLogFileManager_ErrorHandler();
        }
    }

    return 0U;
}

CanLogControlDataType *
CanLogHandler_Init(uint8_t *mount_res, bool *run, bool *commit)
{
    memset(&CanLogCtrlData, 0x0, sizeof(CanLogCtrlData));
    CanLogManager_ResetBufferTelemetry(0U);

    CanLogCtrlData.CanLog.filename    = CanLogFileName;
    CanLogCtrlData.CanLog.fnamemaxlen = sizeof(CanLogFileName);
    CanLogCtrlData.CanLog.openRes     = 1;

    CanLogCtrlData.mountRes     = mount_res;
    CanLogCtrlData.runCanTracer = run;
    CanLogCtrlData.commitLog    = commit;

    CanLogCtrlData.Can1.busLoad         = 0.0;
    CanLogCtrlData.Can1.prevTimestamp   = 0;
    CanLogCtrlData.Can1.accumulatedBits = 0;
    CanLogCtrlData.Can1.accumulatedTime = 0;

    CanLogCtrlData.Can2.busLoad         = 0.0;
    CanLogCtrlData.Can2.prevTimestamp   = 0;
    CanLogCtrlData.Can2.accumulatedBits = 0;
    CanLogCtrlData.Can2.accumulatedTime = 0;

    CanLogPreallocErrorCount = 0U;
    CanLogSdWriteMaxUs       = 0U;
    CanLogSdSyncMaxUs        = 0U;
    CanLogSdStoreBlockMaxUs  = 0U;
    RuntimeChecks_Init();
    CanLogManager_FileLockInit();

    (void)ScInit(&CAN1_Sequence, 0U, UINT32_MAX);
    (void)ScInit(&CAN2_Sequence, 0U, UINT32_MAX);
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if CAN_STATIC_TX_REPLAY_ENABLE
    CanLogManager_StaticReplayIdCheckInit(
        &CAN1_IdReplayCheck,
        CAN_STATIC_TX_REPLAY_CAN1_BASE_ID,
        CAN_STATIC_TX_REPLAY_ID_COUNT
    );
    CanLogManager_StaticReplayIdCheckInit(
        &CAN2_IdReplayCheck,
        CAN_STATIC_TX_REPLAY_CAN2_BASE_ID,
        CAN_STATIC_TX_REPLAY_ID_COUNT
    );
#else
    (void)ScInit(
        &CAN1_IdSequence,
        DEBUG_CAN_ID_SEQUENCE_MAX_ID,
        DEBUG_CAN_ID_SEQUENCE_MAX_ID
    );
    (void)ScInit(
        &CAN2_IdSequence,
        DEBUG_CAN_ID_SEQUENCE_MAX_ID,
        DEBUG_CAN_ID_SEQUENCE_MAX_ID
    );
#endif
#endif
    return &CanLogCtrlData;
}

#if CANLOGMANAGER_CLEAR_ALL_LOGS

static FRESULT delete_all_files(const char *path)
{
    DIR dir;
    FILINFO fno;
    FRESULT res;
    char full_path[256];

    // Open directory
    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        return res;
    }

    // Read all entries
    while (1)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
        {
            break; // End of dir
        }

        // Skip directories (optional - remove if you want to delete subdirs too)
        if (fno.fattrib & AM_DIR)
        {
            continue;
        }

        // Build full path
        sprintf(full_path, "%s/%s", path, fno.fname);

        // Delete the file
        res = f_unlink(full_path);
        if (res != FR_OK)
        {
            f_closedir(&dir);
            return res; // Return on error
        }
    }

    f_closedir(&dir);
    return FR_OK;
}

#endif

static bool m_verify_preallocation(const char *path)
{
    FIL fil;
    FRESULT res;
    UINT bytes_read;
    UINT bytes_to_read;
    BYTE dummy_bytes[1U] = {0};
    uint32_t log_file_size;
    uint32_t seek_offset;
    bool can_seek;
    bool verified;

    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK)
    {
        return false;
    }

    // Try seeking to near the expected pre-allocated size
    log_file_size = appCanLogGetLogFileSize();
    if (log_file_size == 0U)
    {
        f_close(&fil);
        return false;
    }

    seek_offset = (log_file_size > sizeof(dummy_bytes))
                      ? (log_file_size - sizeof(dummy_bytes))
                      : 0U;
    res         = f_lseek(&fil, seek_offset);
    can_seek    = (res == FR_OK);

    bytes_to_read = sizeof(dummy_bytes);
    res           = f_read(&fil, dummy_bytes, bytes_to_read, &bytes_read);
    verified      = (res == FR_OK && bytes_read > 0U);

    f_close(&fil);
    return can_seek;
}

static bool CanLogManager_ParseMarkerValue(
    char *buffer,
    uint32_t buffer_len,
    const char *key,
    uint32_t *out_value
)
{
    JSONStatus_t json_res;
    char tmp[16];
    char *value;
    size_t value_length;

    json_res = FileHandler_GetValue(
        buffer,
        buffer_len,
        key,
        (uint32_t)strlen(key),
        &value,
        &value_length
    );
    if (JSONSuccess != json_res || value_length >= sizeof(tmp))
    {
        return false;
    }

    memcpy(tmp, value, value_length);
    tmp[value_length] = '\0';

    return 0U == FileHandler_ConvertToInteger(tmp, out_value, 10U);
}

static bool CanLogManager_PreallocMarkerMatches(
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    FRESULT res;
    FatFsDeviceType marker;
    uint32_t file_size             = 0U;
    uint32_t read_size             = 0U;
    uint32_t marker_version        = 0U;
    uint32_t marker_log_file_size  = 0U;
    uint32_t marker_log_file_count = 0U;
    char buffer[CAN_LOG_PREALLOC_MARKER_MAX_SIZE];
    bool matches = false;

    res = FatFS_SD_OpenFileForRead(&marker, CAN_LOG_PREALLOC_MARKER_FILENAME);
    if (FR_OK != res)
    {
        return false;
    }

    do
    {
        res = FatFS_SD_GetFileSize(&marker, &file_size);
        if (FR_OK != res || file_size == 0U || file_size >= sizeof(buffer))
        {
            break;
        }

        read_size = file_size;
        res       = FatFS_SD_ReadFile(&marker, buffer, read_size);
        if (FR_OK != res)
        {
            break;
        }

        buffer[read_size] = '\0';
        matches           = CanLogManager_ParseMarkerValue(
                      buffer,
                      read_size,
                      "version",
                      &marker_version
                  )
                  && CanLogManager_ParseMarkerValue(
                      buffer,
                      read_size,
                      "log_file_size",
                      &marker_log_file_size
                  )
                  && CanLogManager_ParseMarkerValue(
                      buffer,
                      read_size,
                      "log_file_count",
                      &marker_log_file_count
                  )
                  && marker_version == CAN_LOG_PREALLOC_MARKER_VERSION
                  && marker_log_file_size == log_file_size
                  && marker_log_file_count == log_file_count;
    } while (0);

    (void)FatFS_SD_CloseFile(&marker);
    return matches;
}

static FRESULT CanLogManager_StorePreallocMarker(
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    FRESULT res;
    FatFsDeviceType marker;
    char content[CAN_LOG_PREALLOC_MARKER_MAX_SIZE];
    int length;

    res = FatFS_SD_OpenFileForOverWrite(
        &marker,
        CAN_LOG_PREALLOC_MARKER_FILENAME
    );
    if (FR_OK != res)
    {
        return res;
    }

    length = snprintf(
        content,
        sizeof(content),
        "{\"version\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}",
        (unsigned long)CAN_LOG_PREALLOC_MARKER_VERSION,
        (unsigned long)log_file_size,
        (unsigned long)log_file_count
    );
    if (length < 0 || (size_t)length >= sizeof(content))
    {
        (void)FatFS_SD_CloseFile(&marker);
        return FR_INVALID_PARAMETER;
    }

    res = FatFS_SD_WriteFile(&marker, content, (uint32_t)length);
    if (FR_OK == res)
    {
        res = FatFS_SD_Flush(&marker);
    }

    if (FR_OK != FatFS_SD_CloseFile(&marker) && FR_OK == res)
    {
        res = FR_DISK_ERR;
    }

    return res;
}

volatile static uint32_t UnseekableFiles = 0;

static FRESULT m_preallocate_log_files(void)
{
    FIL logfile;
    FRESULT res;
    UINT bytes_written;
    BYTE dummy_byte = 0;
    char full_path[256];
    uint32_t log_file_size;
    uint32_t log_file_count;
    uint32_t max_index;
    bool preallocation_complete = false;
    bool prealloc_marker_valid  = false;

    UnseekableFiles          = 0;
    CanLogPreallocErrorCount = 0U;

    log_file_size  = appCanLogGetLogFileSize();
    log_file_count = appCanLogGetLogFileCount();
    max_index      = appCanLogGetMaxLogIndex();

    if (log_file_size == 0U || log_file_count == 0U)
    {
        return FR_INVALID_PARAMETER;
    }

    snprintf(
        full_path,
        sizeof(full_path),
        FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
        (int)max_index
    );

    if (CanLogManager_PreallocMarkerMatches(log_file_size, log_file_count)
        && m_verify_preallocation(full_path))
    {
        preallocation_complete = true;
        prealloc_marker_valid  = true;
    }

    if (!preallocation_complete)
    {
        if (1U == m_verify_preallocation(full_path))
        {
            preallocation_complete = true;
        }
    }

    if (!preallocation_complete)
    {
        for (uint32_t i = 0; i < log_file_count; i++)
        {
            bool prealloc_ok = true;

            snprintf(
                full_path,
                sizeof(full_path),
                FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
                (int)i
            );

            // Check if file already exists and is properly sized
            if (1U == m_verify_preallocation(full_path))
            {
                // File exists and is properly sized - skip
                continue;
            }

            UnseekableFiles++;

            // File doesn't exist or is too small - create/resize it
            res = f_open(&logfile, full_path, FA_WRITE | FA_CREATE_NEW);
            if (res == FR_EXIST)
            {
                // File exists but is too small - open for expansion
                res = f_open(&logfile, full_path, FA_WRITE);
            }

            if (res != FR_OK)
            {
                CanLogPreallocErrorCount++;
                continue;
            }

            if (res == FR_OK)
            {
                res = f_expand(&logfile, log_file_size, 0);

                if (res == FR_OK)
                {
                    res = f_lseek(&logfile, log_file_size - 1U);

                    if (res != FR_OK)
                    {
                        res = f_lseek(&logfile, log_file_size - 512U);
                    }
                    if (res != FR_OK)
                    {
                        res = f_lseek(&logfile, log_file_size - 1024U);
                    }
                    if (res != FR_OK)
                    {
                        res = f_lseek(&logfile, log_file_size - 3U * 512U);
                    }
                    if (res != FR_OK)
                    {
                        res = f_lseek(&logfile, log_file_size - 4U * 512U);
                    }
                    if (res != FR_OK)
                    {
                        res = f_lseek(&logfile, log_file_size - 5U * 512U);
                    }
                }

                if (res == FR_OK)
                {
                    bytes_written = 0U;
                    res = f_write(&logfile, &dummy_byte, 1U, &bytes_written);
                    if (res == FR_OK && bytes_written != 1U)
                    {
                        prealloc_ok = false;
                    }
                }

                if (res == FR_OK && bytes_written == 1)
                {
                    res = f_lseek(&logfile, 0);
                }
            }

            if (res != FR_OK)
            {
                prealloc_ok = false;
            }

            if (f_close(&logfile) != FR_OK)
            {
                prealloc_ok = false;
            }

            if (!prealloc_ok)
            {
                CanLogPreallocErrorCount++;
            }
        }
    }

    if (CanLogPreallocErrorCount == 0U && !prealloc_marker_valid)
    {
        res = CanLogManager_StorePreallocMarker(log_file_size, log_file_count);
        if (FR_OK != res)
        {
            return res;
        }
    }

    CanLogCtrlData.CanLog.fileHeadIndex = 0;

    // Build candidate filename
    snprintf(
        CanLogCtrlData.CanLog.filename,
        CanLogCtrlData.CanLog.fnamemaxlen,
        FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
        0
    );

    if (CanLogPreallocErrorCount != 0U)
    {
        return FR_DISK_ERR;
    }

    CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        CanLogCtrlData.CanLog.filename
    );

    return CanLogCtrlData.CanLog.openRes;
}

#if CANLOGAMANGER_PERSIST_METADATA

static int CanLogManager_ParseMetaData(
    char *buffer,
    uint32_t len,
    CanLogMetaDataType *meta
)
{
    // Variables used in this example.
    JSONStatus_t result;
    char TmpBuf[64];
    char *value;
    size_t valueLength;
    size_t bufferLength = len;
    uint32_t Epoch;
    const char queryKey1[]       = "epoch";
    const size_t queryKeyLength1 = sizeof(queryKey1) - 1;
    uint32_t FileIndex;
    const char queryKey2[]       = "index";
    const size_t queryKeyLength2 = sizeof(queryKey2) - 1;
    uint32_t ByteOffset;
    const char queryKey3[]       = "offset";
    const size_t queryKeyLength3 = sizeof(queryKey3) - 1;
    uint32_t Crc;
    const char queryKey4[]       = "crc";
    const size_t queryKeyLength4 = sizeof(queryKey4) - 1;

    result = JSON_Validate(buffer, bufferLength);

    if (result == JSONSuccess)
    {
        result = FileHandler_GetValue(
            buffer,
            bufferLength,
            queryKey1,
            queryKeyLength1,
            &value,
            &valueLength
        );
        if (JSONSuccess == result)
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &Epoch, 10U))
            {
                meta->epoch = Epoch;
            }
        }

        result = FileHandler_GetValue(
            buffer,
            bufferLength,
            queryKey2,
            queryKeyLength2,
            &value,
            &valueLength
        );
        if (JSONSuccess == result)
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &FileIndex, 10U))
            {
                meta->fileIndex = FileIndex;
            }
        }

        result = FileHandler_GetValue(
            buffer,
            bufferLength,
            queryKey3,
            queryKeyLength3,
            &value,
            &valueLength
        );
        if (JSONSuccess == result)
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &ByteOffset, 10U))
            {
                meta->byteOffset = ByteOffset;
            }
        }

        result = FileHandler_GetValue(
            buffer,
            bufferLength,
            queryKey4,
            queryKeyLength4,
            &value,
            &valueLength
        );
        if (JSONSuccess == result)
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &Crc, 10U))
            {
                meta->crc = Crc;
            }
        }
    }

    if (JSONSuccess == result)
    {
        return CLB_E_OK;
    }
    else
    {
        return CLB_E_NOT_OK;
    }
}

static FRESULT m_MetaDataLoad(CanLogMetaDataType *data)
{
    FRESULT res;
    FILINFO fno;
    FatFsDeviceType Dev;
    char Content[128];
    uint32_t BufferSize;
    uint32_t FileSize;
    uint32_t ReadSize;
    ErrorContextType ErrorContext;

    res = f_stat(CAN_LOG_META_FILENAME, &fno);

    switch (res)
    {
    case FR_OK:
        res = FatFS_SD_OpenFileForRead(&Dev, CAN_LOG_META_FILENAME);
        break;
    case FR_NO_FILE:
    case FR_NO_PATH:
    default:
        break;
    }

    if (FR_OK == res)
    {
        BufferSize = sizeof(Content);

        res = FatFS_SD_GetFileSize(&Dev, &FileSize);
        if (res != FR_OK)
        {
            res = CLB_E_FILE_READ;
        }

        if (CLB_E_OK == res)
        {
            // Read data
            ReadSize = (FileSize < BufferSize) ? FileSize : BufferSize;
            res      = FatFS_SD_ReadFile(&Dev, Content, ReadSize);

            if (CLB_E_OK != res)
            {
                res = CLB_E_FILE_READ;
            }
        }

        if (FR_OK == res)
        {
            res = CanLogManager_ParseMetaData(Content, ReadSize, data);
        }

        res = FatFS_SD_CloseFile(&Dev);

        if (FR_OK != res)
        {
            ErrorContext.code = res;
            ErrorContext.line = __LINE__;
            snprintf(
                ErrorContext.function,
                sizeof(ErrorContext.function),
                "%s",
                "SD_Spi_writeMultiBlock"
            );

            CanLogFileManager_ErrorHandler();
        }
    }
    else
    {
        res = CLB_E_FILE_OPEN;
    }

    return res;
}

static uint8_t m_MetaDataToJSonString(
    CanLogMetaDataType *data,
    char *json,
    uint32_t maxLength,
    uint32_t *len
)
{
    // Create the JSON string
    snprintf(
        json,
        maxLength,
        "{\n"
        "    \"epoch\":%" PRIu32 ",\n"
        "    \"index\":%" PRIu32 ",\n"
        "    \"offset\":%" PRIu32 ",\n"
        "    \"crc\":%" PRIu32 "\n"
        "}\n",
        data->epoch,
        data->fileIndex,
        data->byteOffset,
        data->crc
    );

    *len = strnlen(json, maxLength);
    return 0U;
}

static FRESULT m_MetaDataStore(CanLogMetaDataType *data)
{
    FRESULT res;
    FatFsDeviceType Dev;
    char Content[128];
    uint32_t BufferSize;
    uint32_t StringSize;
    uint32_t WriteSize;

    res = FatFS_SD_OpenFileForOverWrite(&Dev, CAN_LOG_META_FILENAME);

    if (FR_OK == res)
    {
        BufferSize = sizeof(Content);

        (void)m_MetaDataToJSonString(data, Content, BufferSize, &StringSize);

        if (CLB_E_OK == res)
        {
            // Read data
            WriteSize = (StringSize < BufferSize) ? StringSize : BufferSize;
            res       = FatFS_SD_WriteFile(&Dev, Content, WriteSize);

            if (CLB_E_OK != res)
            {
                res = CLB_E_FILE_WRITE;
                CanLogFileManager_ErrorHandler();
            }
        }
        else
        {
            CanLogFileManager_ErrorHandler();
        }

        FatFS_SD_CloseFile(&Dev);
    }
    else
    {
        res = CLB_E_FILE_OPEN;
    }

    return res;
}
#endif

FRESULT appCanLogHandlerInit(CanLogControlDataType *data)
{
    FILINFO info;
    FRESULT res;

#if CANLOGAMANGER_PERSIST_METADATA
    memset(&LogMetaData, 0, sizeof(LogMetaData));

    LogMetaDataOpenRes = m_MetaDataLoad(&LogMetaData);

    if (0 != LogMetaDataOpenRes)
    {
        CanLogFileManager_ErrorHandler();
    }

    LogMetaData.epoch++;

    CanLogBuffer_SetEpochCount(LogMetaData.epoch);

    m_MetaDataStore(&LogMetaData);
#endif

    *CanLogCtrlData.runCanTracer           = false;
    CanLogCtrlData.runCanTracerOld         = false;
    CanLogCtrlData.CanLog.fileHeadIndex    = 0;
    CanLogCtrlData.CanLog.fileTailIndex    = 0;
    CanLogCtrlData.CanLog.fileIndexWrapped = 0;

    CanLogBuffer_Init();
    CanLogManager_ResetBufferTelemetry(CanLogManager_ReadRb1UsedBytes());

    fdcan_msg_port_init();

#if CANLOGMANAGER_CLEAR_ALL_LOGS
    res = f_stat(FILEHANDLER_PARTITION_NO "/logs", &info);

    if ((res == FR_OK) && (info.fattrib & AM_DIR))
    {
        if (FR_OK == delete_all_files(FILEHANDLER_PARTITION_NO "/logs"))
        {
            f_rmdir(FILEHANDLER_PARTITION_NO "/logs");
        }
    }
#endif

    res = f_stat(FILEHANDLER_PARTITION_NO "/logs", &info);

    if ((res == FR_OK) && (info.fattrib & AM_DIR))
    {
    }
    else if (res == FR_NO_FILE)
    {
        // Directory does not exist
        res = f_mkdir(FILEHANDLER_PARTITION_NO "/logs");
        if (res != FR_OK)
        {
            CanLogFileManager_ErrorHandler();
        }
    }
    else if ((res == FR_OK) && (!(info.fattrib & AM_DIR)))
    {
        CanLogFileManager_ErrorHandler();
    }

#if PREALLOCATE_LOG_FILES
    if (RES_OK == *CanLogCtrlData.mountRes)
    {
        res = m_preallocate_log_files();
        if (res != FR_OK)
        {
            CanLogFileManager_ErrorHandler();
        }
    }
    else
    {
    }
#endif

    if (RES_OK == *CanLogCtrlData.mountRes)
    {
        appCanLogOpenMostRecentFile(data);
    }
    else
    {
        CanLogCtrlData.CanLog.openRes = 1U;
    }

    return res;
}

static void appCanLogFillEntry(
    CanLogEntryType *entry,
    FDCAN_ClassicFrameType *frame,
    uint64_t timestamp,
    uint8_t channel
)
{
    ClbDlcFlagsType Flags = 0;
    uint8_t Dlc;
    uint8_t DataLen;

    Dlc   = FDCAN_GET_DLC(frame);
    Flags = FDCAN_GET_BRS(frame) << CAN_FLAG_BRS_Pos
            | FDCAN_GET_ESI(frame) << CAN_FLAG_ESI_Pos
            | FDCAN_GET_FDF(frame) << CAN_FLAG_RTR_FDF_Pos
            | FDCAN_GET_IDE(frame) << CAN_FLAG_IDE_Pos;
    DataLen = FDCAN_GET_DATA_LEN(frame);

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->header.total_len  = sizeof(CanLogEntryType) + DataLen;
    entry->timestamp         = timestamp;
    entry->channel           = channel;
    entry->dlc_flags         = MAKE_DLC_FLAGS(Dlc, Flags);
    entry->data_len          = DataLen;
    entry->can_id            = frame->id;

    memcpy(entry->data, frame->data, DataLen);
}

#if INSTR_ENABLED
#if INSTR_PERSIST_ACTIVE
static InstrErrorType appPersistInstrumentationData(void)
{
    FRESULT res;
    FatFsDeviceType File;
    uint32_t BytesToWrite = 0;
    uint8_t *Data         = NULL;
    const char FileName[] = "InstrumentationData.bin";

    res = FatFS_SD_OpenFileForOverWrite(&File, FileName);

    if (FR_OK == res)
    {
        Instrumentation_SerializeHook(Data, &BytesToWrite);
        FatFS_SD_WriteFile(&File, (const char *)Data, BytesToWrite);
    }

    FatFS_SD_CloseFile(&File);

    return res;
}
#endif
#endif

static comm_status_t appCanLogStoreToFrameBuffer(void *entry)
{
    comm_status_t res        = COMM_SUCCESS;
    ClbReturnType buffer_res = CLB_E_OK;
    CanLogEntryHeaderType *pHeader;

    pHeader = (CanLogEntryHeaderType *)entry;

    buffer_res = CanLogBuffer_AddEntry(entry, pHeader->total_len);

    if (CLB_E_OK != buffer_res)
    {
        CanLogManager_FrameDropCount1++;

        if (CLB_E_BUFFER_FULL == buffer_res)
        {
            CanLogManager_FrameDropCount2++;
        }

        res = COMM_ERROR;
    }

    CanLogManager_UpdateRb1BytesHighWater();

    return res;
}

static comm_status_t appCanLogStoreToSd(
    FatFsDeviceType *dev,
    char *data,
    uint32_t length,
    uint32_t *write_us,
    uint32_t *sync_us,
    uint32_t *store_block_us
)
{
    comm_status_t res = COMM_SUCCESS;
    uint64_t start_us;
    uint64_t after_write_us;
    uint64_t after_sync_us;

    if ((NULL == write_us) || (NULL == sync_us) || (NULL == store_block_us))
    {
        return COMM_NULL_POINTER;
    }

    *write_us       = 0U;
    *sync_us        = 0U;
    *store_block_us = 0U;

    start_us = FDCAN_GetTimestampHook();
    if (FR_OK == FatFS_SD_WriteFile(dev, (const char *)data, length))
    {
        after_write_us = FDCAN_GetTimestampHook();
        if (FR_OK != FatFS_SD_Flush(dev))
        {
            res = COMM_ERROR;
        }
        after_sync_us = FDCAN_GetTimestampHook();

        *write_us       = (uint32_t)(after_write_us - start_us);
        *sync_us        = (uint32_t)(after_sync_us - after_write_us);
        *store_block_us = (uint32_t)(after_sync_us - start_us);

        if (*write_us > CanLogSdWriteMaxUs)
        {
            CanLogSdWriteMaxUs = *write_us;
        }
        if (*sync_us > CanLogSdSyncMaxUs)
        {
            CanLogSdSyncMaxUs = *sync_us;
        }
        if (*store_block_us > CanLogSdStoreBlockMaxUs)
        {
            CanLogSdStoreBlockMaxUs = *store_block_us;
        }
    }
    else
    {
        after_write_us  = FDCAN_GetTimestampHook();
        *write_us       = (uint32_t)(after_write_us - start_us);
        *store_block_us = *write_us;
        if (*write_us > CanLogSdWriteMaxUs)
        {
            CanLogSdWriteMaxUs = *write_us;
        }
        res = COMM_ERROR;
    }

    (void)FatFS_SD_GetBufferedFileSize(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        &LogMetaData.byteOffset
    );

    LogMetaData.fileIndex = CanLogCtrlData.CanLog.fileHeadIndex;

    return res;
}

static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev)
{
    uint32_t DataLength;
    uint32_t FrameCount;
    uint32_t Rb1BytesBeforeWrite;
    uint32_t Rb1BytesAfterWriteBeforeConsume;
    uint32_t Rb1BytesAfterConsume;
    uint32_t SdWriteUs;
    uint32_t SdSyncUs;
    uint32_t SdStoreBlockUs;
    uint8_t *DataPtr;
    comm_status_t res = COMM_SUCCESS;

    if (CLB_E_OK
        == CanLogBuffer_ReadNextBlock(&DataPtr, &DataLength, &FrameCount))
    {
        Rb1BytesBeforeWrite = CanLogManager_ReadRb1UsedBytes();
        CanLogManager_RecordRb1UsedBytes(Rb1BytesBeforeWrite);
        if (Rb1BytesBeforeWrite > Rb1BytesHighWater)
        {
            Rb1BytesHighWater = Rb1BytesBeforeWrite;
        }
        Rb1BytesAtLastSdBlockStart = Rb1BytesBeforeWrite;

        CanLogManager_InstrumentationFlushStartHook();
        CanLogManager_FileLock();
        res = appCanLogStoreToSd(
            dev,
            (char *)DataPtr,
            DataLength,
            &SdWriteUs,
            &SdSyncUs,
            &SdStoreBlockUs
        );
        if (COMM_SUCCESS == res)
        {
            (void)appCanLogCheckNewFileOpenLocked();
        }
        CanLogManager_FileUnlock();
        CanLogManager_InstrumentationFlushEndHook();

        Rb1BytesAfterWriteBeforeConsume = CanLogManager_ReadRb1UsedBytes();
        CanLogManager_RecordRb1UsedBytes(Rb1BytesAfterWriteBeforeConsume);
        if (Rb1BytesAfterWriteBeforeConsume > Rb1BytesHighWater)
        {
            Rb1BytesHighWater = Rb1BytesAfterWriteBeforeConsume;
        }
        Rb1BytesAtLastSdBlockEndBeforeConsume = Rb1BytesAfterWriteBeforeConsume;

        if (COMM_SUCCESS == res)
        {
            CanLogBuffer_Consume(DataLength, FrameCount);
            CanLogBuffer_BlockCount++;
            Rb1BytesAfterConsume = CanLogManager_ReadRb1UsedBytes();
        }
        else
        {
            Rb1BytesAfterConsume = Rb1BytesAfterWriteBeforeConsume;
        }

        CanLogManager_RecordRb1UsedBytes(Rb1BytesAfterConsume);
        Rb1BytesAtLastSdBlockEndAfterConsume = Rb1BytesAfterConsume;
        CanLogManager_RecordSdBlockTiming(
            SdWriteUs,
            SdSyncUs,
            SdStoreBlockUs,
            FrameCount,
            res
        );
    }
    else
    {
        res = COMM_ERROR;
    }

    return res;
}

static comm_status_t CanLogManager_EmitSyncEntry(
    CanLogSyncType *sync,
    uint64_t absTime,
    uint32_t frameTimestamp
)
{
    /* Keep the 32-bit timestamp aligned with the originating frame to avoid
     * offsets when the absolute 64-bit timer wraps. */
    sync->timestamp     = CLM_ABS_TIME_TO_TIMSTAMP(frameTimestamp);
    sync->abs_time_high = CLM_ABS_TIME_TO_ABS_HIGH(absTime);

    sync->header.header_len = sizeof(sync->header);
    sync->header.type       = CLB_ENTRY_TYPE_SYNC;
    sync->header.total_len  = sizeof(CanLogSyncType);

    return appCanLogStoreToFrameBuffer((void *)sync);
}

void SdBridgeTask_ActionHook(void)
{
    uint8_t block_ready = 0U;

    do
    {
        block_ready = 0U;
        if (CLB_E_OK != CanLogBuffer_IsBlockReady(&block_ready))
        {
            CanLogFileManager_ErrorHandler();
            break;
        }

        if (0U != block_ready)
        {
            if (COMM_SUCCESS
                != appCanLogStoreBlock(&(CanLogCtrlData.CanLog.writeFileDevice)
                ))
            {
                CanLogFileManager_ErrorHandler();
                break;
            }
        }
    } while (0U != block_ready);
}

CanLogResult m_ComputeFrameBits(CanLogEntryType *frame, uint32_t *bits)
{
    CanLogResult res = CAN_LOG_OK;
    uint8_t Dlc;
    uint32_t FrameBits = 0;
    bool IsCanfd;
    bool IsExtended;

    Dlc = GET_DLC(frame->dlc_flags) >> CAN_DLC_MASK_Pos;

    switch (Dlc)
    {
    case 9:
        Dlc = 12;
        break;
    case 10:
        Dlc = 16;
        break;
    case 11:
        Dlc = 20;
        break;
    case 12:
        Dlc = 24;
        break;
    case 13:
        Dlc = 32;
        break;
    case 14:
        Dlc = 48;
        break;
    case 15:
        Dlc = 64;
        break;
    default:
        break;
    }

    IsCanfd    = (frame->dlc_flags & CAN_FLAG_RTR_FDF);
    IsExtended = (frame->dlc_flags & CAN_FLAG_IDE);

    if (IsCanfd)
    {
        // CAN FD frame structure
        if (IsExtended)
        {
            // Extended ID: 1 + 32 + 2 + 1 + 1 + 4 + data + CRC + 2 + 7 + 3 = 53 + data + CRC
            FrameBits = 53 + (Dlc * 8);
            // CRC: 17 bits for ≤16 bytes, 21 bits for >16 bytes
            FrameBits += (Dlc <= 16) ? 17 : 21;
        }
        else
        {
            // Standard ID: 1 + 12 + 2 + 1 + 1 + 4 + data + CRC + 2 + 7 + 3 = 33 + data + CRC
            FrameBits = 33 + (Dlc * 8);
            FrameBits += (Dlc <= 16) ? 17 : 21;
        }
    }
    else
    {
        // Classic CAN frame structure
        if (IsExtended)
        {
            // Extended: 1 + 32 + 6 + data + 15 + 1 + 2 + 7 + 3 = 67 + data
            FrameBits = 67 + (Dlc * 8);
        }
        else
        {
            // Standard: 1 + 11 + 6 + data + 15 + 1 + 2 + 7 + 3 = 47 + data
            FrameBits = 47 + (Dlc * 8);
        }
    }

    *bits = FrameBits;

    return res;
}

CanLogResult m_ComputeBusLoad(CanStatusDataType *can, CanLogEntryType *frame)
{
    CanLogResult res   = CAN_LOG_OK;
    float Period       = 0.0f; // in seconds
    uint32_t FrameBits = 0;
    uint32_t TimeDiff  = 0;
    uint32_t ts;
    uint32_t prev;
    uint32_t diff;

    if (NULL == can || NULL == frame)
    {
        res = CAN_LOG_ERR_INVALID_POINTER;
        return res;
    }
    else if (can->baudrate == 0)
    {
        res = CAN_LOG_ERR_INVALID_PARAM;
        return res;
    }

    if (can->prevTimestamp == 0)
    {
        can->prevTimestamp = frame->timestamp;
        return res;
    }

    ts   = frame->timestamp;
    prev = can->prevTimestamp;
    diff = 0U;

    if (ts >= prev)
    {
        diff               = ts - prev;
        can->prevTimestamp = ts;
    }
    else // prev > ts
    {
        if ((prev > UINT32_MAX / 2U) && (ts < UINT32_MAX / 2U))
        {
            diff               = (UINT32_MAX - prev) + ts + 1U;
            can->prevTimestamp = ts;
        }
    }

    TimeDiff = diff;

    m_ComputeFrameBits(frame, &FrameBits);

    can->accumulatedBits += FrameBits;
    can->accumulatedTime += TimeDiff;

    // Update every 1 second
    if (can->accumulatedTime >= 1000000U)
    {
        Period = can->accumulatedTime / 1000000.0f;
        can->busLoad =
            ((can->accumulatedBits * 1.1f) / (can->baudrate * Period)) * 100.0f;

        if (can->busLoad > 0.0f && can->busLoad < 0.01f)
        {
            can->busLoad = 0.01f;
        }

        can->accumulatedBits = 0;
        can->accumulatedTime = 0;
    }

    return res;
}

CanLogResult m_ComputeBusLoad1(CanStatusDataType *can, CanLogEntryType *frame)
{
    return m_ComputeBusLoad(can, frame);
}

CanLogResult m_ComputeBusLoad2(CanStatusDataType *can, CanLogEntryType *frame)
{
    return m_ComputeBusLoad(can, frame);
}

void m_DecayBusLoad(CanStatusDataType *can, uint32_t currentTime)
{
    int64_t TimeDiff;

    TimeDiff = currentTime - can->prevTimestamp;

    if (TimeDiff < 0)
    {
        TimeDiff = (uint32_t)((int64_t)UINT32_MAX + TimeDiff) + 1U;
    }

    if (TimeDiff > 1000000)
    {
        can->busLoad *= 0.75f;
        if (can->busLoad < 0.01f)
        {
            can->busLoad = 0.0f;
        }
        can->accumulatedBits = 0;
        can->accumulatedTime = 0;
    }
}

ClmErrorType appCanLogHandlerPoll(CanLogControlDataType *data)
{
    ClmErrorType RetVal = CLM_E_OK;
    comm_status_t res   = COMM_SUCCESS;
    bool IsOffState;
    uint8_t BlockIsReady  = 0U;
    uint32_t SlotsToWrite = 0;
    FDCAN_ClassicFrameType *pNewFrame;
    volatile uint64_t AbsTime = 0;
    RuntimeChecksContextType ErrorContext;
    CanLogSyncType SyncEntry;
    CanLogEntryStackBufferType EntryBuffer;
    CanLogEntryType *pFrameEntry       = (CanLogEntryType *)(&EntryBuffer);
    uint32_t CAN1_MissingFrames        = 0U;
    uint32_t CAN2_MissingFrames        = 0U;
    static uint32_t CAN1_SequenceIndex = 0U;
    static uint32_t CAN2_SequenceIndex = 0U;
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if !CAN_STATIC_TX_REPLAY_ENABLE
    static uint32_t CAN1_CanId = DEBUG_CAN_ID_SEQUENCE_MAX_ID;
    static uint32_t CAN2_CanId = DEBUG_CAN_ID_SEQUENCE_MAX_ID;
#endif
#endif
    volatile uint32_t LocalFrameCount;
    static uint64_t NextPeriodicSyncAbsTime = 0;
    static uint32_t LastFrameTimestamp      = 0;
    static bool LastFrameTimestampValid     = false;

    if (CanLogCtrlData.runCanTracerOld == *CanLogCtrlData.runCanTracer)
    {
    }
    else if (true == *CanLogCtrlData.runCanTracer)
    {
        CanLogManager_ResetBufferTelemetry(CanLogManager_ReadRb1UsedBytes());
#if CANLOGMANAGER_REOPEN_LOG_FILE
        if (CAN_LOG_OK != appCanLogReopenFile(data))
        {
            CanLogFileManager_ErrorHandler();
        }
#endif

        CanLogCtrlData.emitSyncEntry = true;
        AbsTime                      = FDCAN_GetTimestampHook();
        NextPeriodicSyncAbsTime      = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
        LastFrameTimestampValid      = false;

        CanLogCtrlData.framesLost = false;
        RuntimeChecks_Init();
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if CAN_STATIC_TX_REPLAY_ENABLE
        CanLogManager_StaticReplayIdCheckInit(
            &CAN1_IdReplayCheck,
            CAN_STATIC_TX_REPLAY_CAN1_BASE_ID,
            CAN_STATIC_TX_REPLAY_ID_COUNT
        );
        CanLogManager_StaticReplayIdCheckInit(
            &CAN2_IdReplayCheck,
            CAN_STATIC_TX_REPLAY_CAN2_BASE_ID,
            CAN_STATIC_TX_REPLAY_ID_COUNT
        );
#else
        CAN1_CanId = DEBUG_CAN_ID_SEQUENCE_MAX_ID;
        CAN2_CanId = DEBUG_CAN_ID_SEQUENCE_MAX_ID;
        ScReset(&CAN1_IdSequence, CAN1_CanId);
        ScReset(&CAN2_IdSequence, CAN2_CanId);
#endif
#endif

        CanAbs_IsStateOff_Can1(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can1())
        {
            res = COMM_ERROR;
            CanLogFileManager_ErrorHandler();
        }

        CanAbs_IsStateOff_Can2(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can2())
        {
            res = COMM_ERROR;
            CanLogFileManager_ErrorHandler();
        }

        if (COMM_SUCCESS != res)
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        CanLogCtrlData.runCanTracerOld = *CanLogCtrlData.runCanTracer;
    }
    else
    {
        if (COMM_SUCCESS != CanAbs_Stop_Can1())
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        if (COMM_SUCCESS != CanAbs_Stop_Can2())
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        CanLogCtrlData.runCanTracerOld = *CanLogCtrlData.runCanTracer;

        *(CanLogCtrlData.commitLog) = true;

        CanAbs_Drain();

        fdcan_msg_port_flush();
    }

    (void)LocalFrameCount;
    LocalFrameCount = 0;

    AbsTime = FDCAN_GetTimestampHook();

    m_DecayBusLoad(&CanLogCtrlData.Can1, CLM_ABS_TIME_TO_TIMSTAMP(AbsTime));
    m_DecayBusLoad(&CanLogCtrlData.Can2, CLM_ABS_TIME_TO_TIMSTAMP(AbsTime));

    if ((0ULL != NextPeriodicSyncAbsTime)
        && (AbsTime >= NextPeriodicSyncAbsTime))
    {
        CanLogCtrlData.emitSyncEntry = true;
        NextPeriodicSyncAbsTime      = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
    }

    while (((CANLOG_INGEST_BATCH_MAX > LocalFrameCount)
            || (false == *CanLogCtrlData.runCanTracer))
           && (0 < fdcan_msg_port_read(&pNewFrame, 2)))
    {
        CanLogManager_DrainPortStartHook();
        CanLogManager_FrameCount++;
        LocalFrameCount++;

        AbsTime = FDCAN_GetTimestampHook();

        if ((0ULL != NextPeriodicSyncAbsTime)
            && (AbsTime >= NextPeriodicSyncAbsTime))
        {
            CanLogCtrlData.emitSyncEntry = true;
            NextPeriodicSyncAbsTime      = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
        }

        if (true == CanLogCtrlData.emitSyncEntry)
        {
            CanLogCtrlData.emitSyncEntry = false;
            if (COMM_SUCCESS
                != CanLogManager_EmitSyncEntry(
                    &SyncEntry,
                    AbsTime,
                    pNewFrame->timestamp
                ))
            {
                CanLogFileManager_ErrorHandler();
            }
        }

        appCanLogFillEntry(
            pFrameEntry,
            pNewFrame,
            pNewFrame->timestamp,
            pNewFrame->channel
        );

        if (pFrameEntry->channel == 1)
        {
            CanLogCtrlData.Can1.receivedFrames++;
            m_ComputeBusLoad1(&CanLogCtrlData.Can1, pFrameEntry);
            CAN1_SequenceIndex = pNewFrame->rx_sequence;
            (void)ScCheckSequence(&CAN1_Sequence, CAN1_SequenceIndex);
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if CAN_STATIC_TX_REPLAY_ENABLE
            if (!CanLogManager_StaticReplayIdCheck(
                    &CAN1_IdReplayCheck,
                    pNewFrame->id
                ))
            {
                CanLogManager_CAN1_MissingIdsCount++;
            }
#else
            CAN1_CanId = pNewFrame->id;
            if (SC_E_SEQUENCE == ScCheckSequence(&CAN1_IdSequence, CAN1_CanId))
            {
                CanLogManager_CAN1_MissingIdsCount++;
            }
#endif
#endif
        }
        else if (pFrameEntry->channel == 2)
        {
            CanLogCtrlData.Can2.receivedFrames++;
            m_ComputeBusLoad2(&CanLogCtrlData.Can2, pFrameEntry);
            CAN2_SequenceIndex = pNewFrame->rx_sequence;
            (void)ScCheckSequence(&CAN2_Sequence, CAN2_SequenceIndex);
#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE
#if CAN_STATIC_TX_REPLAY_ENABLE
            if (!CanLogManager_StaticReplayIdCheck(
                    &CAN2_IdReplayCheck,
                    pNewFrame->id
                ))
            {
                CanLogManager_CAN2_MissingIdsCount++;
            }
#else
            CAN2_CanId = pNewFrame->id;
            if (SC_E_SEQUENCE == ScCheckSequence(&CAN2_IdSequence, CAN2_CanId))
            {
                CanLogManager_CAN2_MissingIdsCount++;
            }
#endif
#endif
        }

        if (COMM_SUCCESS != appCanLogStoreToFrameBuffer((void *)pFrameEntry))
        {
            *CanLogCtrlData.runCanTracer = false;
            CanLogFileManager_ErrorHandler();
            break;
        }
        LastFrameTimestamp      = pNewFrame->timestamp;
        LastFrameTimestampValid = true;

        CanLogBuffer_IsBlockReady(&BlockIsReady);

        if (0 != CanLogCtrlData.CanLog.openRes)
        {
            *CanLogCtrlData.runCanTracer = false;
            CanLogFileManager_ErrorHandler();
            break;
        }
        else if (BlockIsReady)
        {
            SdBridgeTask_Notify();
        }
        else
        {
        }
        CanLogManager_DrainPortEndHook();
    }

    if (SC_E_OK == ScGetMissingCount(&CAN1_Sequence, &CAN1_MissingFrames))
    {
        CanLogManager_CAN1_MissingCount += CAN1_MissingFrames;
        ScReset(&CAN1_Sequence, CAN1_SequenceIndex);
    }

    if (SC_E_OK == ScGetMissingCount(&CAN2_Sequence, &CAN2_MissingFrames))
    {
        CanLogManager_CAN2_MissingCount += CAN2_MissingFrames;
        ScReset(&CAN2_Sequence, CAN2_SequenceIndex);
    }

#if DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE && !CAN_STATIC_TX_REPLAY_ENABLE
    uint32_t CAN1_MissingIds = 0U;
    uint32_t CAN2_MissingIds = 0U;

    if (SC_E_OK == ScGetMissingCount(&CAN1_IdSequence, &CAN1_MissingIds))
    {
        CanLogManager_CAN1_MissingIdsCount += CAN1_MissingIds;
        ScReset(&CAN1_IdSequence, CAN1_CanId);
    }

    if (SC_E_OK == ScGetMissingCount(&CAN2_IdSequence, &CAN2_MissingIds))
    {
        CanLogManager_CAN2_MissingIdsCount += CAN2_MissingIds;
        ScReset(&CAN2_IdSequence, CAN2_CanId);
    }
#endif

    if (BlockIsReady)
    {
        RetVal = CLM_E_BLOCK_READY;
    }

    if (true == CanLogCtrlData.emitSyncEntry)
    {
        uint32_t timestamp32 = LastFrameTimestampValid
                                   ? LastFrameTimestamp
                                   : CLM_ABS_TIME_TO_TIMSTAMP(AbsTime);

        CanLogCtrlData.emitSyncEntry = false;
        if (COMM_SUCCESS
            != CanLogManager_EmitSyncEntry(&SyncEntry, AbsTime, timestamp32))
        {
            CanLogFileManager_ErrorHandler();
        }
    }

    if (*(CanLogCtrlData.commitLog))
    {
        CanLogBuffer_UsedSlots((uint8_t *)&SlotsToWrite);

        if (SlotsToWrite > 0)
        {
            if (CLB_E_OK != CanLogBuffer_FillBlockWithPadding())
            {
                CanLogFileManager_ErrorHandler();
            }
            CanLogManager_UpdateRb1BytesHighWater();
            SdBridgeTask_Notify();
        }

#if CANLOGMANAGER_REOPEN_LOG_FILE
        if (CAN_LOG_OK != appCanLogCloseFile(data))
        {
            CanLogFileManager_ErrorHandler();
        }
#endif

#if INSTR_ENABLED
#if INSTR_PERSIST_ACTIVE
        appPersistInstrumentationData();
#endif
#endif

        *(CanLogCtrlData.commitLog) = false;
    }

    (void)RuntimeChecks_CheckFrameCounts(&ErrorContext);

    if (RUNTIMECHECKS_E_FRAMES_DROPPED == ErrorContext.err)
    {
        CanLogCtrlData.framesLost = true;
    }

    return RetVal;
}

void appCanLogHandlerDeInit(CanLogControlDataType *data)
{
    (void)data;

    if (0 == CanLogCtrlData.CanLog.openRes)
    {
        FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice));
    }

    (void)ScDeInit(&CAN1_Sequence);

    (void)ScDeInit(&CAN2_Sequence);
}
