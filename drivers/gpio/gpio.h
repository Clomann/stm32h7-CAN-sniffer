#pragma once

#include <stdint.h>

#define DBG_PIN                                GPIO_PIN_2
#define DBG_GPIO_PORT                          GPIOB
#define DBG_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define DBG_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOB_CLK_DISABLE()

#define MCO1_PIN                                GPIO_PIN_8
#define MCO1_GPIO_PORT                          GPIOA
#define MCO1_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOA_CLK_ENABLE()

int32_t GPIO_Dbg_Init();

int32_t GPIO_Dbg_DeInit();

int32_t GPIO_Dbg_On();

int32_t GPIO_Dbg_Off();

int32_t GPIO_Dbg_Toggle();

int32_t GPIO_Dbg_GetState ();

int32_t GPIO_Mco1_Init();