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

/**
 * Initialize timer with a provided resolution.
 * 
 * \param[in] Timer resolution in micro seconds per tick.
 */
uint8_t TIMx_Init(uint32_t resolution);
void TIMx_IRQHandler(void);
void TIM_GetCounterValue(uint16_t *cnt);
void TIM_GetArrValue(uint16_t *arr);

/* Shims needed to be implemented by caller */
void TIM_InterruptCallback(void);
void TIM_ErrorHandler(void);