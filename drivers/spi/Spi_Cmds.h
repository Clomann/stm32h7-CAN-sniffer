/*
 * Spi_Cmds.h
 *
 *  Created on: Dec 4, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_SPI_CMDS_H_
#define CM7_INC_SPI_CMDS_H_

#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

#include "CommFactory.h"
#include "SpiCfg.h"

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

extern uint8_t __dma_buffers_start; /* see linker script */
extern uint8_t __dma_buffers_end; /* see linker script */

extern const uint8_t aTxSpiInit[18];
extern const uint8_t aRxSpiDummy[512];

static inline uint8_t Spi_PwrOn()
{
	HAL_GPIO_WritePin(SPI1_PWR_GPIO_PORT, SPI1_PWR_PIN, GPIO_PIN_SET);
	return 0;
}

static inline uint8_t Spi_PwrOff()
{
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_PWR_PIN, GPIO_PIN_RESET);
	return 0;
}

static inline uint8_t Spi_CsEnable()
{
#if CS_ACTIVE_HIGH
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
#else
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
#endif
	return 0;
}

static inline uint8_t Spi_CsDisable()
{
#if CS_ACTIVE_HIGH
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
#else
	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
#endif
	return 0;
}

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *);

uint8_t Spi_readByte(SPI_HandleTypeDef * handle, uint8_t * pResponse);
uint8_t Spi_writByte(SPI_HandleTypeDef * handle, const uint8_t *data);
uint8_t Spi_PollForResponse(SPI_HandleTypeDef * handle, uint8_t * pResponse);
uint8_t Spi_Send(SPI_HandleTypeDef * handle, uint8_t * buffer, uint16_t len);
uint8_t Spi_Receive(SPI_HandleTypeDef * handle, uint8_t * buffer, uint16_t len);
uint8_t Spi_SendReceiveMsg(SPI_HandleTypeDef * handle, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint16_t TxBytes);
uint8_t Spi_goHighSpeed(SPI_HandleTypeDef * handle);

uint8_t Spi_NotifyTransferIssued(SPI_HandleTypeDef *hspi);
uint8_t Spi_NotifyTransferComplete(SPI_HandleTypeDef *hspi);
uint8_t Spi_NotifyTransferError(SPI_HandleTypeDef *hspi);
uint8_t Spi_NotifyRxData(SPI_HandleTypeDef *hspi, uint8_t err);

void Spi_ErrorHandler(void);

void SPI1_DMA_RX_IRQHandler(void);
void SPI1_DMA_TX_IRQHandler(void);

void Spi_Lock(uint8_t id);
void Spi_Unlock(uint8_t id);

#endif /* CM7_INC_SPI_CMDS_H_ */
