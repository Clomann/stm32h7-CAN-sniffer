#pragma once

#include "stm32h7xx_hal.h"

static inline int osDelay(uint32_t delay)
{
    HAL_Delay(delay);
    return 0U;
}
