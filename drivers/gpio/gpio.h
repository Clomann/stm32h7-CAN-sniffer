#pragma once

#include <stdint.h>

#define DBG_PIN                                GPIO_PIN_2
#define DBG_GPIO_PORT                          GPIOB
#define DBG_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define DBG_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOB_CLK_DISABLE()

int32_t GPIO_Dbg_Init();

int32_t GPIO_Dbg_DeInit();

int32_t GPIO_Dbg_On();

int32_t GPIO_Dbg_Off();

int32_t GPIO_Dbg_Toggle();

int32_t GPIO_Dbg_GetState ();