#include "instrumentation.h"

#include "gpio.h"

#include <stdbool.h>

void CanAbs_InstrumentationIsrStartHook(void);
void CanAbs_InstrumentationIsrEndHook(void);
void FileHandler_InstrumentationWriteStartHook(void);
void FileHandler_InstrumentationWriteEndHook(void);
void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);

#define INSTR_CAN_ISR_PIN       DBG_PIN_1
#define INSTR_SD_WRITE_PIN      DBG_PIN_2
#define INSTR_BLOCK_FLUSH_PIN   DBG_PIN_3

static bool InstrumentationReady = false;

static inline bool instr_ready(void)
{
    return InstrumentationReady;
}

void Instrumentation_Init(void)
{
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

    InstrumentationReady = true;
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

    GPIO_Dbg_On(INSTR_SD_WRITE_PIN);
}

void FileHandler_InstrumentationWriteEndHook(void)
{
    if (!instr_ready())
    {
        return;
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
