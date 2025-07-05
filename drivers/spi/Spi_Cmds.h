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

#include "SpiCfg.h"

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

static const uint8_t aTxSpiInit[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t aTxSpiDummy1[] = {0xFF};
static  const uint8_t aTxSpiDummy4[] = {0xFF, 0xFF, 0xFF, 0xFF};

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

HAL_StatusTypeDef SPI_Init(void);
//static uint8_t Spi_Receive(uint8_t *r, uint8_t);
uint8_t Spi_readByte(uint8_t * pResponse);
uint8_t Spi_writByte(const uint8_t *data);
uint8_t Spi_PollForResponse(uint8_t *);
uint8_t Spi_PollTillIdle(uint8_t *);
uint8_t Spi_ParseResponse(const uint8_t *, uint8_t, uint8_t *);
uint8_t Spi_SendReceiveMsg(const uint8_t *, uint8_t *, uint8_t);

void SPI1_DMA_RX_IRQHandler(void);
void SPI1_DMA_TX_IRQHandler(void);

#endif /* CM7_INC_SPI_CMDS_H_ */
