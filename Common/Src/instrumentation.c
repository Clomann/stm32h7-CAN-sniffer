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
    uint64_t timestamp; /*<! in micro seconds */
    uint32_t timedelta; /*<! wraps at ~71 minutes */
} InstrumentationTimeTraceType;

#define INSTR_TRACE_COUNT_MAX 1000U
#define INSTR_CAN_ISR_PIN     DBG_PIN_1
#define INSTR_DRAIN_PORT_PIN  DBG_PIN_4
#define INSTR_SD_WRITE_PIN    DBG_PIN_2
#define INSTR_BLOCK_FLUSH_PIN DBG_PIN_3

static ErrorContextType ErrorContext = {.file = __FILE_NAME__, .code = 0};

static lwrb_t FileHandlerTraceBuffer;
static InstrumentationTimeTraceType __attribute__((
    section(".ram_d3")
)) FileHandlerTraceData[INSTR_TRACE_COUNT_MAX] = {0};

static bool InstrumentationReady    = false;
static uint64_t last_sd_write_start = 0;

void CanAbs_InstrumentationIsrStartHook(void);
void CanAbs_InstrumentationIsrEndHook(void);
void FileHandler_InstrumentationWriteStartHook(void);
void FileHandler_InstrumentationWriteEndHook(void);
void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);
void CanLogManager_DrainPortStartHook(void);

__attribute__((weak)) void
Instrumentation_ErrorHandlerHook(ErrorContextType *context)
{
    (void)context;
}

#if INSTR_PERSIST_ACTIVE

#define INSTR_DATA_CONTENT ""

__attribute__((weak)) InstrErrorType
Instrumentation_SerializeHook(uint8_t *buf, uint32_t *size)
{
    InstrErrorType res;

    (void)buf;

    buf   = (uint8_t *)FileHandlerTraceData;
    *size = sizeof(FileHandlerTraceData);

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
    if (1
        != lwrb_init(
            &FileHandlerTraceBuffer,
            (void *)FileHandlerTraceData,
            sizeof(FileHandlerTraceData)
        ))
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
    last_sd_write_start = Instrumentation_GetTimestampUs();
    GPIO_Dbg_On(INSTR_SD_WRITE_PIN);
}

void FileHandler_InstrumentationWriteEndHook(void)
{
    uint8_t res = 0;
    InstrumentationTimeTraceType TraceEntry;

    if (!instr_ready())
    {
        return;
    }

    TraceEntry.timestamp = Instrumentation_GetTimestampUs();
    TraceEntry.timedelta =
        (uint32_t)(TraceEntry.timestamp - last_sd_write_start);

    res = Instrumentation_WriteEntryToBuffer(
        &FileHandlerTraceBuffer,
        (const void *)&TraceEntry,
        sizeof(TraceEntry)
    );

    if (0 != res)
    {
        strncpy(
            ErrorContext.function,
            "FileHandler_InstrumentationWriteEndHook",
            sizeof(ErrorContext.function)
        );
        ErrorContext.line = __LINE__;
        Instrumentation_ErrorHandlerHook(&ErrorContext);
    }

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
