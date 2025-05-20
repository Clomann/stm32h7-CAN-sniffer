#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

/* Definition for TIMx clock resources */
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()


/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler


uint8_t TIMx_Init(uint32_t prescaler);
void TIMx_IRQHandler(void);

/* Shims needed to be implemented by caller */
void TIM_InterruptCallback(void);
void TIM_ErrorHandler(void);