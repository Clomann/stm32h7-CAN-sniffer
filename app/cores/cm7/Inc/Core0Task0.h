#pragma once

#include "main.h"

#include <stdint.h>

#define WAIT_FOR_USER_BUTTON (0U)
#define TIM_HAL_TIME_FREQ    (1000000U)

void Core0Task0Init(void);

void DEFERRED_IRQn_Handler(void);
