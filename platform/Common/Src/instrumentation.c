#include "instrumentation.h"

#include "ErrorContext.h"
#include "buffers.h"
#include "gpio.h"
#include "fdcan.h"
#include "lwrb/lwrb.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    uint32_t timestamp; /*<! in micro seconds; wraps at ~71 minutes */
    uint32_t value;
} InstrumentationTimeTraceType;

#define INSTR_TRACE_COUNT_MAX 4002U
#define INSTR_CAN_ISR_PIN     DBG_PIN_1
#define INSTR_DRAIN_PORT_PIN  DBG_PIN_4
#define INSTR_SD_WRITE_PIN    DBG_PIN_2
#define INSTR_BLOCK_FLUSH_PIN DBG_PIN_3

#define TRACE_SELECTED(component)            (COMPONENT_TRACE == (component))
#define INSTR_FDCAN_CENTER_TRIGGER_MIN_VALUE (12480U)

#ifndef INSTR_FDCAN_FREEZE_TRIGGER_VALUE
#ifdef INSTR_FDCAN_CENTER_TRIGGER_MIN_VALUE
#define INSTR_FDCAN_FREEZE_TRIGGER_VALUE INSTR_FDCAN_CENTER_TRIGGER_MIN_VALUE
#else
#define INSTR_FDCAN_FREEZE_TRIGGER_VALUE UINT32_MAX
#endif
#endif

#define INSTR_FDCAN_TRIGGER_PRE_COUNT ((INSTR_TRACE_COUNT_MAX - 1U) / 2U)
#define INSTR_FDCAN_TRIGGER_POST_COUNT                                         \
    (INSTR_TRACE_COUNT_MAX - INSTR_FDCAN_TRIGGER_PRE_COUNT - 1U)

#if TRACE_SELECTED(FILEHANDLER_TRACE)
static ErrorContextType ErrorContext = {.file = __FILE_NAME__, .code = 0};
#endif

static lwrb_t TraceBuffer;
static InstrumentationTimeTraceType
    __attribute__((section(".ram_d3"))) TraceData[INSTR_TRACE_COUNT_MAX] = {0};

static bool InstrumentationReady = false;
#if TRACE_SELECTED(FILEHANDLER_TRACE)
static uint64_t last_sd_write_start = 0;
#endif
#if TRACE_SELECTED(FDCANMSGPORT_TRACE)
typedef enum
{
    FDCAN_TRACE_ROLLING,
    FDCAN_TRACE_POST_TRIGGER,
    FDCAN_TRACE_FROZEN
} FdcanTraceStateType;

static FdcanTraceStateType FdcanTraceState = FDCAN_TRACE_ROLLING;
static uint32_t FdcanTracePeakValue        = 0U;
static uint32_t FdcanTracePreWriteIndex    = 0U;
static uint32_t FdcanTracePreValidCount    = 0U;
static uint32_t FdcanTraceWriteIndex       = 0U;
static uint32_t FdcanTracePostCount        = 0U;
static uint32_t FdcanTraceStoredCount      = 0U;
#endif

void CanAbs_InstrumentationIsrStartHook(void);
void CanAbs_InstrumentationIsrEndHook(void);
void FileHandler_InstrumentationWriteStartHook(void);
void FileHandler_InstrumentationWriteEndHook(void);
void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);
void CanLogManager_DrainPortStartHook(void);
void FdcanMsgPort_InstrumentationUsedBytesHook(uint32_t used_bytes);
void CanLogManager_InstrumentationFdcanMsgPortPeakHook(
    uint32_t timestamp_us,
    uint32_t used_bytes
);

__attribute__((weak)) void
Instrumentation_ErrorHandlerHook(ErrorContextType *context)
{
    (void)context;
}

__attribute__((weak)) void CanLogManager_InstrumentationFdcanMsgPortPeakHook(
    uint32_t timestamp_us,
    uint32_t used_bytes
)
{
    (void)timestamp_us;
    (void)used_bytes;
}

#if INSTR_PERSIST_ACTIVE

#define INSTR_DATA_CONTENT ""

__attribute__((weak)) InstrErrorType
Instrumentation_SerializeHook(uint8_t *buf, uint32_t *size)
{
    InstrErrorType res;

    (void)buf;

    buf = (uint8_t *)TraceData;
#if TRACE_SELECTED(FDCANMSGPORT_TRACE)
    *size = FdcanTraceStoredCount * sizeof(TraceData[0]);
#else
    *size = sizeof(TraceData);
#endif

    res = INSTR_E_OK;

    return res;
}
#endif

uint64_t Instrumentation_GetTimestampUs(void);

static inline bool instr_ready(void)
{
    return InstrumentationReady;
}

void Instrumentation_Init(void)
{
    if (1 != lwrb_init(&TraceBuffer, (void *)TraceData, sizeof(TraceData)))
    {
        return;
    }

    if (GPIO_Dbg_Init(INSTR_CAN_ISR_PIN) != BSP_ERROR_NONE)
    {
        return;
    }

    if (GPIO_Dbg_Init(INSTR_SD_WRITE_PIN) != BSP_ERROR_NONE)
    {
        return;
    }

    if (GPIO_Dbg_Init(INSTR_BLOCK_FLUSH_PIN) != BSP_ERROR_NONE)
    {
        return;
    }

    if (GPIO_Dbg_Init(INSTR_DRAIN_PORT_PIN) != BSP_ERROR_NONE)
    {
        return;
    }

    InstrumentationReady = true;
}

uint64_t Instrumentation_GetTimestampUs(void)
{
    return FDCAN_GetTimestampHook(); // or replicate the TIMx logic
}

#if TRACE_SELECTED(FILEHANDLER_TRACE)
static InstrErrorType
Instrumentation_WriteEntryToBuffer(lwrb_t *buf, const void *entry, uint32_t btw)
{
    InstrErrorType res = INSTR_E_OK;
    lwrb_sz_t BytesWritten;

    if (btw > lwrb_get_free(buf))
    {
        lwrb_skip(buf, btw);
    }

    BytesWritten = lwrb_write(buf, entry, btw);

    if (BytesWritten != btw)
    {
        res = INSTR_E_NOT_OK;
    }

    return res;
}

static void Instrumentation_WriteTraceEntry(
    uint32_t timestamp,
    uint32_t value,
    const char *function
)
{
    uint8_t res = 0;
    InstrumentationTimeTraceType TraceEntry;

    TraceEntry.timestamp = timestamp;
    TraceEntry.value     = value;

    res = Instrumentation_WriteEntryToBuffer(
        &TraceBuffer,
        (const void *)&TraceEntry,
        sizeof(TraceEntry)
    );

    if (0 != res)
    {
        strncpy(ErrorContext.function, function, sizeof(ErrorContext.function));
        ErrorContext.line = __LINE__;
        Instrumentation_ErrorHandlerHook(&ErrorContext);
    }
}
#endif

#if TRACE_SELECTED(FDCANMSGPORT_TRACE)
static bool Instrumentation_FdcanTraceTriggerAllowed(uint32_t value)
{
#if INSTR_FDCAN_FREEZE_TRIGGER_VALUE > 0U
    return value >= INSTR_FDCAN_FREEZE_TRIGGER_VALUE;
#else
    (void)value;
    return true;
#endif
}

static void
Instrumentation_FdcanTraceStoreRolling(const InstrumentationTimeTraceType *entry
)
{
    TraceData[FdcanTracePreWriteIndex] = *entry;
    FdcanTracePreWriteIndex++;

    if (FdcanTracePreWriteIndex >= INSTR_FDCAN_TRIGGER_PRE_COUNT)
    {
        FdcanTracePreWriteIndex = 0U;
    }

    if (FdcanTracePreValidCount < INSTR_FDCAN_TRIGGER_PRE_COUNT)
    {
        FdcanTracePreValidCount++;
    }

    FdcanTraceStoredCount = FdcanTracePreValidCount;
}

static void Instrumentation_FdcanTraceLinearizePreTrigger(void)
{
    uint32_t start;
    uint32_t i;
    uint32_t source_index;

    if (FdcanTracePreValidCount == INSTR_FDCAN_TRIGGER_PRE_COUNT)
    {
        start = FdcanTracePreWriteIndex;
    }
    else
    {
        start = 0U;
    }

    for (i = 0U; i < FdcanTracePreValidCount; i++)
    {
        source_index = start + i;
        if (source_index >= INSTR_FDCAN_TRIGGER_PRE_COUNT)
        {
            source_index -= INSTR_FDCAN_TRIGGER_PRE_COUNT;
        }

        TraceData[INSTR_FDCAN_TRIGGER_PRE_COUNT + 1U + i] =
            TraceData[source_index];
    }

    for (i = 0U; i < FdcanTracePreValidCount; i++)
    {
        TraceData[i] = TraceData[INSTR_FDCAN_TRIGGER_PRE_COUNT + 1U + i];
    }
}

static void Instrumentation_FdcanTraceStartPostTrigger(
    const InstrumentationTimeTraceType *entry
)
{
    Instrumentation_FdcanTraceLinearizePreTrigger();
    CanLogManager_InstrumentationFdcanMsgPortPeakHook(
        entry->timestamp,
        entry->value
    );

    TraceData[FdcanTracePreValidCount] = *entry;
    FdcanTraceWriteIndex               = FdcanTracePreValidCount + 1U;
    FdcanTracePostCount                = 0U;
    FdcanTraceStoredCount              = FdcanTraceWriteIndex;
    FdcanTraceState                    = FDCAN_TRACE_POST_TRIGGER;
}

static void Instrumentation_FdcanTraceRecenterOnNewPeak(
    const InstrumentationTimeTraceType *entry
)
{
    uint32_t keep_count;

    CanLogManager_InstrumentationFdcanMsgPortPeakHook(
        entry->timestamp,
        entry->value
    );

    keep_count = FdcanTraceWriteIndex;
    if (keep_count > INSTR_FDCAN_TRIGGER_PRE_COUNT)
    {
        keep_count = INSTR_FDCAN_TRIGGER_PRE_COUNT;
    }

    if (keep_count > 0U)
    {
        memmove(
            &TraceData[0],
            &TraceData[FdcanTraceWriteIndex - keep_count],
            keep_count * sizeof(TraceData[0])
        );
    }

    TraceData[keep_count] = *entry;
    FdcanTraceWriteIndex  = keep_count + 1U;
    FdcanTracePostCount   = 0U;
    FdcanTraceStoredCount = FdcanTraceWriteIndex;
}

static void Instrumentation_FdcanTraceStorePostTrigger(
    const InstrumentationTimeTraceType *entry
)
{
    if (FdcanTraceWriteIndex >= INSTR_TRACE_COUNT_MAX)
    {
        FdcanTraceState = FDCAN_TRACE_FROZEN;
        return;
    }

    TraceData[FdcanTraceWriteIndex] = *entry;
    FdcanTraceWriteIndex++;
    FdcanTracePostCount++;
    FdcanTraceStoredCount = FdcanTraceWriteIndex;

    if (FdcanTracePostCount >= INSTR_FDCAN_TRIGGER_POST_COUNT)
    {
        FdcanTraceState = FDCAN_TRACE_FROZEN;
    }
}

static void Instrumentation_FdcanTraceEntry(uint32_t timestamp, uint32_t value)
{
    InstrumentationTimeTraceType TraceEntry;

    TraceEntry.timestamp = timestamp;
    TraceEntry.value     = value;

    if (FdcanTraceState == FDCAN_TRACE_ROLLING)
    {
        if (Instrumentation_FdcanTraceTriggerAllowed(value))
        {
            FdcanTracePeakValue = value;
            Instrumentation_FdcanTraceStartPostTrigger(&TraceEntry);
        }
        else
        {
            Instrumentation_FdcanTraceStoreRolling(&TraceEntry);
        }
    }
    else if (FdcanTraceState == FDCAN_TRACE_POST_TRIGGER)
    {
        if (value >= FdcanTracePeakValue)
        {
            FdcanTracePeakValue = value;
            Instrumentation_FdcanTraceRecenterOnNewPeak(&TraceEntry);
        }
        else
        {
            Instrumentation_FdcanTraceStorePostTrigger(&TraceEntry);
        }
    }
    else
    {
    }
}
#endif

void CanAbs_InstrumentationIsrStartHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_On(INSTR_CAN_ISR_PIN);
}

void CanAbs_InstrumentationIsrEndHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_Off(INSTR_CAN_ISR_PIN);
}

void FileHandler_InstrumentationWriteStartHook(void)
{
    if (!instr_ready())
    {
        return;
    }
#if TRACE_SELECTED(FILEHANDLER_TRACE)
    last_sd_write_start = Instrumentation_GetTimestampUs();
#endif
    GPIO_Dbg_On(INSTR_SD_WRITE_PIN);
}

void FileHandler_InstrumentationWriteEndHook(void)
{
#if TRACE_SELECTED(FILEHANDLER_TRACE)
    uint32_t timestamp;
#endif

    if (!instr_ready())
    {
        return;
    }

#if TRACE_SELECTED(FILEHANDLER_TRACE)
    timestamp = (uint32_t)Instrumentation_GetTimestampUs();
    Instrumentation_WriteTraceEntry(
        timestamp,
        (uint32_t)(timestamp - last_sd_write_start),
        "FileHandler_InstrumentationWriteEndHook"
    );
#endif
    GPIO_Dbg_Off(INSTR_SD_WRITE_PIN);
}

void CanLogManager_InstrumentationFlushStartHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_On(INSTR_BLOCK_FLUSH_PIN);
}

void CanLogManager_InstrumentationFlushEndHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_Off(INSTR_BLOCK_FLUSH_PIN);
}

void CanLogManager_DrainPortStartHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_On(INSTR_DRAIN_PORT_PIN);
}

void CanLogManager_DrainPortEndHook(void)
{
    if (!instr_ready())
    {
        return;
    }

    GPIO_Dbg_Off(INSTR_DRAIN_PORT_PIN);
}

void FdcanMsgPort_InstrumentationUsedBytesHook(uint32_t used_bytes)
{
#if TRACE_SELECTED(FDCANMSGPORT_TRACE)
    if (!instr_ready())
    {
        return;
    }

    Instrumentation_FdcanTraceEntry(
        (uint32_t)Instrumentation_GetTimestampUs(),
        used_bytes
    );
#else
    (void)used_bytes;
#endif
}
