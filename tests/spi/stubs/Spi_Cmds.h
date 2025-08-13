#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

#include "CommFactory.h"
#include "SpiCfg.h"

#ifndef SPI_1
#define SPI_1 ((void*)0x40013000)
#endif

#ifndef SPI_2
#define SPI_2 ((void*)0x40003800)
#endif

extern volatile int Spi_Init_return_value;
extern volatile int Spi_Init_call_count;
extern volatile int Spi_SendReceiveMsg_call_count;
extern uint8_t Spi_SendReceiveMsg_last_tx[];
extern uint8_t Spi_SendReceiveMsg_last_rx[];
extern size_t Spi_SendReceiveMsg_last_len;

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *handle);

HAL_StatusTypeDef Spi_SendReceiveMsg(SPI_HandleTypeDef * handle, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint16_t TxBytes);

void Spi_ErrorHandler(void);