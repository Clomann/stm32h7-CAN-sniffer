#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

/* Definition for TIMx clock resources and definition for TIMx's NVIC */
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler

#define TIM_HAL                        TIM4
#define TIM_HAL_CLK_ENABLE()           __HAL_RCC_TIM4_CLK_ENABLE()
#define TIM_HAL_IRQn                   TIM4_IRQn
#define TIM_HAL_IRQHandler             TIM4_IRQHandler

/**
 * Initialize timer with a provided resolution.
 * 
 * \param[in] resolution of timer ticks in micro seconds per tick.
 */
uint8_t TIMx_Init(uint32_t resolution);
void TIMx_IRQHandler(void);
void TIM_GetCounterValue(uint16_t *cnt);
void TIM_GetArrValue(uint16_t *arr);

/**
 * Initialize timer with a provided resolution.
 * 
 * \param[in] resolution in mHz.
 */
uint8_t TIM_HAL_Init(uint32_t frequency);
void TIM_HAL_IRQHandler(void);

/* Shims needed to be implemented by caller */
void TIM_InterruptCallback(void);
void TIM_ErrorHandler(void);

void TIM_HAL_InterruptCallback(void);
void TIM_HAL_ErrorHandler(void);

void TIM_ErrorHandlerHook(void);