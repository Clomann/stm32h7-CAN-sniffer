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

/* SPI handler declaration */
static SPI_HandleTypeDef *pSpiHandle1;

/* Buffer used for transmission */
ALIGN_32BYTES(uint8_t __attribute__((section(".dma_buffer"))) aTxBuffer[]) = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // "****SPI - Two Boards communication based on DMA **** SPI Message ********* SPI Message *********";

/* Buffer used for reception */
/* Size of buffer */
#define BUFFERSIZE              (COUNTOF(aTxBuffer) - 1)
#define BUFFER_ALIGNED_SIZE 	(((BUFFERSIZE+31)/32)*32)
ALIGN_32BYTES(uint8_t __attribute__((section(".dma_buffer"))) aRxBuffer[BUFFER_ALIGNED_SIZE]);

uint8_t Spi_PollTillIdle(SPI_HandleTypeDef * handle, uint8_t *);
uint8_t Spi_ParseResponse(const uint8_t *, uint8_t, uint8_t *);
static void Error_Handler(void);
void SPI1_DMA_RX_IRQHandler(void);
void SPI1_DMA_TX_IRQHandler(void);

static uint8_t m_NotifyTransferComplete(SPI_HandleTypeDef *hspi);
static uint8_t m_NotifyTransferError(SPI_HandleTypeDef *hspi);
static uint8_t m_NotifyTransferIssued(SPI_HandleTypeDef *hspi);

static inline uint8_t is_in_dma_nocache(const void *addr, size_t len)
{
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end   = start + len - 1U;

    return  (uint8_t)((start >= (uintptr_t)&__dma_buffers_start) &&
            (end   <  (uintptr_t)&__dma_buffers_end));
}

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
//	RetVal = HAL_SPI_Receive_DMA(pSpiHandle1, pRxBuffer, RxBytes);
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
//	while (HAL_SPI_GetState(pSpiHandle1) != HAL_SPI_STATE_READY) {}
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

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef * handle)
{   
    HAL_StatusTypeDef res;

    if (NULL == handle)
    {
        Spi_ErrorHandler();
    }

    pSpiHandle1 = handle;

	/* Set the SPI1 parameters */
	handle->Instance               = SPI1;
	handle->Init.Mode              = SPI_MODE_MASTER;
	handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	handle->Init.Direction         = SPI_DIRECTION_2LINES;
	handle->Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
	handle->Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
	handle->Init.DataSize          = SPI_DATASIZE_8BIT;
	handle->Init.FirstBit          = SPI_FIRSTBIT_MSB;
	handle->Init.TIMode            = SPI_TIMODE_DISABLE;
	handle->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	handle->Init.CRCPolynomial     = 7;
	handle->Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	handle->Init.NSS               = SPI_NSS_SOFT;
	handle->Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	handle->Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */
	res = HAL_SPI_Init(handle);
    return res;
}

uint8_t Spi_Send(SPI_HandleTypeDef * handle, uint8_t * pTxBuffer, uint8_t TxBytes)
{
	uint8_t RetVal;

	RetVal = HAL_SPI_Transmit_DMA(handle, pTxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
		/* Transfer error in transmission process */
		Error_Handler();
	}

    m_NotifyTransferIssued(handle);

	// Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(handle) != HAL_SPI_STATE_READY) {}

	return RetVal;
}

uint8_t Spi_SendReceiveMsg(SPI_HandleTypeDef * handle, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint16_t TxBytes)
{
	uint8_t RetVal;

    Spi_Lock(0);

    if (!is_in_dma_nocache((void*)pTxBuffer, TxBytes))
	{
        SCB_CleanDCache_by_Addr ((uint32_t *)pTxBuffer, TxBytes);
    }

	RetVal = HAL_SPI_TransmitReceive_DMA(handle, pTxBuffer, aRxBuffer, TxBytes);

	if (RetVal == HAL_BUSY)
	{

	}
	else if (RetVal != HAL_OK)
	{
	  /* Transfer error in transmission process */
		Error_Handler();
        Spi_Unlock(0);
        return HAL_ERROR;
	}

    m_NotifyTransferIssued(handle);

    if (!is_in_dma_nocache((void*)aRxBuffer, TxBytes))
	{
        SCB_InvalidateDCache_by_Addr ((uint32_t *)aRxBuffer, TxBytes);
    }
    
	memcpy(pRxBuffer, aRxBuffer, TxBytes);
    
    Spi_Unlock(0);

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

uint8_t Spi_readByte(SPI_HandleTypeDef * handle, uint8_t * pResponse)
{
	uint8_t RetVal;

	RetVal = Spi_SendReceiveMsg(handle, (uint8_t*)aTxSpiDummy1, (uint8_t *)aRxBuffer, COUNTOF(aTxSpiDummy1));

	Spi_ParseResponse(aRxBuffer, COUNTOF(aTxSpiDummy1), pResponse);

	return RetVal;
}

uint8_t Spi_writByte(SPI_HandleTypeDef * handle, const uint8_t *data)
{
	uint8_t RetVal;
	uint8_t RespDummy = 0U;

	(void) RespDummy;

	RetVal = Spi_SendReceiveMsg(handle, (uint8_t const *)data, (uint8_t *)&RespDummy, 1U);

	return RetVal;
}

uint8_t Spi_PollForResponse(SPI_HandleTypeDef * handle, uint8_t * pResponse)
{
	uint8_t NoResponseReceived;
	uint8_t RetVal;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	counter = 0;
	NoResponseReceived = 1;

	do
	{
		Spi_readByte(handle, pResponse);

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

uint8_t Spi_PollTillIdle(SPI_HandleTypeDef * handle, uint8_t * pResponse)
{
	uint8_t NoResponseReceived;
	uint8_t RetVal;
	uint8_t counter;
	const uint8_t RetryCount = 10;

	counter = 0;
	NoResponseReceived = 1;

	do
	{
		Spi_readByte(handle, pResponse);

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

uint8_t Spi_goHighSpeed(SPI_HandleTypeDef * handle)
{
	uint8_t res;

	res = 0U;

	if(HAL_SPI_DeInit(handle) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

    /* Set the SPI1 parameters */
	handle->Instance               = SPI1;
	handle->Init.Mode              = SPI_MODE_MASTER;
	handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	handle->Init.Direction         = SPI_DIRECTION_2LINES;
	handle->Init.CLKPhase          = SPI_PHASE_1EDGE;  // CPHA = 0: Data captured on the rising edge
	handle->Init.CLKPolarity       = SPI_POLARITY_LOW;  // CPOL = 0: Clock is low when idle
	handle->Init.DataSize          = SPI_DATASIZE_8BIT;
	handle->Init.FirstBit          = SPI_FIRSTBIT_MSB;
	handle->Init.TIMode            = SPI_TIMODE_DISABLE;
	handle->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	handle->Init.CRCPolynomial     = 7;
	handle->Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	handle->Init.NSS               = SPI_NSS_SOFT;
	handle->Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	handle->Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommended setting to avoid glitches */
	res = HAL_SPI_Init(handle);

	if(res != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	return res;
}

uint8_t m_NotifyTransferIssued(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    Spi_NotifyTransferIssued(hspi);
    return res;
}

uint8_t m_NotifyTransferComplete(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    Spi_NotifyTransferComplete(hspi);
    Spi_NotifyRxData(hspi, 0);
    return res;
}

uint8_t m_NotifyTransferError(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    Spi_NotifyTransferError(hspi);
    Spi_NotifyRxData(hspi, 1);
    return res;
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
  
  m_NotifyTransferComplete(hspi);
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
  m_NotifyTransferError(hspi);
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

/**
  * @brief  This function handles SPI1 interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(pSpiHandle1);
}

/**
  * @brief  This function handles DMA Rx interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_DMA_RX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(pSpiHandle1->hdmarx);
}

/**
  * @brief  This function handles DMA Tx interrupt request.
  * @param  None
  * @retval None
  */
void SPI1_DMA_TX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(pSpiHandle1->hdmatx);
}

