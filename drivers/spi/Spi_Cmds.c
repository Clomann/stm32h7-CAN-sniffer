/*
 * Spi_Cmds.c
 *
 *  Created on: Dec 14, 2024
 *      Author: cbromann
 */

#include <string.h>

#include "Spi_Cmds.h"

/* Private define ------------------------------------------------------------*/
enum {
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};

uint8_t spi1_tx_buffer[SPI1_TX_BUFFER_SIZE];
uint8_t spi1_rx_buffer[SPI1_RX_BUFFER_SIZE];
uint8_t spi2_tx_buffer[SPI2_TX_BUFFER_SIZE];
uint8_t spi2_rx_buffer[SPI2_RX_BUFFER_SIZE];

/* SPI handler declaration */
SPI_HandleTypeDef SpiHandle1;

/* transfer state */
__IO uint32_t wTransferState = TRANSFER_WAIT;

/* Buffer used for transmission */
uint8_t aTxBuffer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // "****SPI - Two Boards communication based on DMA **** SPI Message ********* SPI Message *********";

/* Buffer used for reception */
/* Size of buffer */
#define BUFFERSIZE              (COUNTOF(aTxBuffer) - 1)
#define BUFFER_ALIGNED_SIZE 	(((BUFFERSIZE+31)/32)*32)
ALIGN_32BYTES(uint8_t aRxBuffer[BUFFER_ALIGNED_SIZE]);


static void Error_Handler(void);

//uint8_t Spi_Receive(uint8_t * pRxBuffer, uint8_t RxBytes)
//{
//	uint8_t RetVal;
//
//#if CS_ACTIVE_HIGH
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
//#else
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
//#endif
//
//	RetVal = HAL_SPI_Receive_DMA(&SpiHandle1, pRxBuffer, RxBytes);
//
//	if (RetVal == HAL_BUSY)
//	{
//
//	}
//	else if (RetVal != HAL_OK)
//	{
//	  /* Transfer error in transmission process */
//	  Error_Handler();
//	}
//
//	while (wTransferState == TRANSFER_WAIT)
//	{
//	}
//
//	// Wait until the SPI is no longer busy
//	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}
//
//#if CS_ACTIVE_HIGH
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_RESET);
//#else
//	HAL_GPIO_WritePin(SPI1_SS_GPIO_PORT, SPI1_SS_PIN, GPIO_PIN_SET);
//#endif
//
//	SCB_InvalidateDCache_by_Addr ((uint32_t *)pRxBuffer, RxBytes);
//
//	return RetVal;
//}

HAL_StatusTypeDef SPI_Init()
{
	/* Set the SPI1 parameters */
	SpiHandle1.Instance               = SPI1;
	SpiHandle1.Init.Mode              = SPI_MODE_MASTER;
#if TEST_SPI_PLL2
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
#else
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
#endif
	SpiHandle1.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle1.Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
	SpiHandle1.Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
	SpiHandle1.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle1.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle1.Init.CRCPolynomial     = 7;
	SpiHandle1.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	SpiHandle1.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	SpiHandle1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */
	return HAL_SPI_Init(&SpiHandle1);
}

uint8_t Spi_Send(uint8_t * pTxBuffer, uint8_t TxBytes)
{
	uint8_t RetVal;

	RetVal = HAL_SPI_Transmit_DMA(&SpiHandle1, pTxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
		/* Transfer error in transmission process */
		Error_Handler();
	}

	while (wTransferState == TRANSFER_WAIT)
	{
	}

	// Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}

	return RetVal;
}

uint8_t Spi_SendReceiveMsg(const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes)
{
	uint8_t RetVal;

	SCB_CleanDCache_by_Addr ((uint32_t *)pTxBuffer, TxBytes);

	RetVal = HAL_SPI_TransmitReceive_DMA(&SpiHandle1, pTxBuffer, aRxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
	  /* Transfer error in transmission process */
		Error_Handler();
	}

	while (wTransferState == TRANSFER_WAIT)
	{
	}

	// Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(&SpiHandle1) != HAL_SPI_STATE_READY) {}

	SCB_InvalidateDCache_by_Addr ((uint32_t *)aRxBuffer, TxBytes);

	memcpy(pRxBuffer, aRxBuffer, TxBytes);

	return RetVal;
}

uint8_t Spi_ParseResponse(const uint8_t * buffer, uint8_t length, uint8_t * response)
{
	uint8_t RetVal = 1;

	*response = 0xFF;

	for (int i=0; i<length; i++)
	{
		if (0xFF != buffer[i])
		{
			*response = buffer[i];
			RetVal = 0;
		}
	}

	return RetVal;
}

uint8_t Spi_readByte(uint8_t * pResponse)
{
	uint8_t RetVal;

	RetVal = Spi_SendReceiveMsg((uint8_t*)aTxSpiDummy1, (uint8_t *)aRxBuffer, COUNTOF(aTxSpiDummy1));

	Spi_ParseResponse(aRxBuffer, COUNTOF(aTxSpiDummy1), pResponse);

	return RetVal;
}

uint8_t Spi_writByte(const uint8_t *data)
{
	uint8_t RetVal;
	uint8_t RespDummy;

	RetVal = Spi_SendReceiveMsg((uint8_t*)data, (uint8_t *)RespDummy, 1U);

	return RetVal;
}

uint8_t Spi_PollForResponse(uint8_t * pResponse)
{
	uint8_t NoResponseReceived;
	uint8_t RetVal;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	counter = 0;
	NoResponseReceived = 1;

	do
	{
		Spi_readByte(pResponse);

		if (0xFF != *pResponse)
		{
			NoResponseReceived = 0;
		}

		counter++;
	} while (NoResponseReceived && (RetryCount > counter) );

	if (0 == NoResponseReceived)
	{
		RetVal = 0;
	}
	else
	{
		RetVal = 1;
	}

	return RetVal;
}

uint8_t Spi_PollTillIdle(uint8_t * pResponse)
{
	uint8_t NoResponseReceived;
	uint8_t RetVal;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	counter = 0;
	NoResponseReceived = 1;

	do
	{
		Spi_readByte(pResponse);

		if (0xFF == *pResponse)
		{
			NoResponseReceived = 0;
		}

		counter++;
	} while (NoResponseReceived && (RetryCount > counter) );

	if (0 == NoResponseReceived)
	{
		RetVal = 0;
	}
	else
	{
		RetVal = 1;
	}

	return RetVal;
}

uint8_t Spi_goHighSpeed()
{
	uint8_t RetVal;

	RetVal = 0U;

	if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/*##-1- Configure the SPI peripheral #######################################*/
	/* Set the SPI1 parameters */
	SpiHandle1.Instance               = SPI1;
	SpiHandle1.Init.Mode              = SPI_MODE_MASTER;
#if TEST_SPI_PLL2
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // <-----
#else
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
#endif
	SpiHandle1.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle1.Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
	SpiHandle1.Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
	SpiHandle1.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle1.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle1.Init.CRCPolynomial     = 7;
	SpiHandle1.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	SpiHandle1.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	SpiHandle1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */

	if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	return RetVal;
}

/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

  /* Turn LED1 on: Transfer in transmission process is complete */
  BSP_LED_On(LED1);
  /* Turn LED2 on: Transfer in reception process is complete */
  BSP_LED_On(LED2);
  wTransferState = TRANSFER_COMPLETE;
}


/**
  * @brief  SPI error callbacks.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  wTransferState = TRANSFER_ERROR;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  BSP_LED_Off(LED1);
  /* Turn LED3 on */
  BSP_LED_On(LED3);

}
