#pragma once

#include <stdint.h>

#include "stm32h745xx.h"
#include "stm32h7xx_nucleo.h"

#define DBG_PIN_1              GPIO_PIN_1
#define DBG_PIN_2              GPIO_PIN_2
#define DBG_PIN_3              GPIO_PIN_3
#define DBG_PIN_4              GPIO_PIN_4
#define DBG_GPIO_PORT          GPIOB
#define DBG_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()
#define DBG_GPIO_CLK_DISABLE() __HAL_RCC_GPIOB_CLK_DISABLE()

#define MCO1_PIN               GPIO_PIN_8
#define MCO1_GPIO_PORT         GPIOA
#define MCO1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

int32_t GPIO_Dbg_Init(uint32_t nbr);

int32_t GPIO_Dbg_DeInit(uint32_t nbr);

int32_t GPIO_Dbg_On(uint32_t nbr);

int32_t GPIO_Dbg_Off(uint32_t nbr);

int32_t GPIO_Dbg_Toggle(uint32_t nbr);

int32_t GPIO_Dbg_GetState(uint32_t nbr);

int32_t GPIO_Mco1_Init();
