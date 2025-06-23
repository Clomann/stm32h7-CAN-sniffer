#pragma once

#include "main.h"

#include <stdint.h>

#define WAIT_FOR_USER_BUTTON	    (0U)
#define TIMx_TIME_RESOLUTION        (1U)
#define TIM_HAL_TIME_FREQ           (1000000U)

void Core0Task0Init(void);