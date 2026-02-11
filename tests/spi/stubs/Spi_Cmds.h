#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

#include "CommFactory.h"
#include "SpiCfg.h"

#ifndef SPI_1
#define SPI_1 ((void *)0x40013000)
#endif

#ifndef SPI_2
#define SPI_2 ((void *)0x40003800)
#endif

extern volatile int Spi_Init_return_value;
extern volatile int Spi_Init_call_count;
extern volatile int Spi_SendReceiveMsg_call_count;
extern uint8_t Spi_SendReceiveMsg_last_tx[];
extern uint8_t Spi_SendReceiveMsg_last_rx[];
extern size_t Spi_SendReceiveMsg_last_len;

extern volatile int Spi_Receive_call_count;
extern size_t Spi_Receive_last_len;
extern uint8_t Spi_Receive_last_rx_buffer[512];

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *handle);

HAL_StatusTypeDef Spi_SendReceiveMsg(
    SPI_HandleTypeDef *handle,
    const uint8_t *pTxBuffer,
    uint8_t *pRxBuffer,
    uint16_t TxBytes
);
uint8_t Spi_Send(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len);
uint8_t Spi_Receive(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len);

void Spi_ErrorHandler(void);
