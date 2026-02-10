#pragma once

#include "nvic_irg_config.h"
#include "Core0TasksCfg.h"

#include "stm32h7xx_hal.h"

void SpiTask_PortInit(void);

void SpiAbs_PortDeInit(void);

void SpiTask_Init(void);

void SpiTask(void *parameters);

__attribute__((weak)) void SpiTask_ErrorHandlerHook(void);
