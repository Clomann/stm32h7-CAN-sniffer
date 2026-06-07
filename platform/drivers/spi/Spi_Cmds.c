/*
 * Spi_Cmds.c
 *
 *  Created on: Dec 14, 2024
 *      Author: cbromann
 */

#include <string.h>

#include "Spi_Cmds.h"
#include "stm32h745xx.h"
#include "stm32h7xx_hal_rcc_ex.h"
#include "spi_utils.h"
#include "memory_sections.h"

/* Private define ------------------------------------------------------------*/
enum
{
    TRANSFER_WAIT,
    TRANSFER_COMPLETE,
    TRANSFER_ERROR
};

/* SPI handler declaration */
static SPI_HandleTypeDef *pSpiHandle1;

/* Buffer used for transmission */
ALIGN_32BYTES(uint8_t DMA_BUFFER aTxBuffer[]) = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
}; // "****SPI - Two Boards communication based on DMA **** SPI Message ********* SPI Message *********";
ALIGN_32BYTES(uint8_t DMA_BUFFER aRxSpiDummy[1024U]);
ALIGN_32BYTES(uint8_t DMA_BUFFER aRxSpiSink[1024U]);

/* Buffer used for reception */
/* Size of buffer */
#define BUFFERSIZE          (COUNTOF(aTxBuffer) - 1)
#define BUFFER_ALIGNED_SIZE (((BUFFERSIZE + 31) / 32) * 32)
ALIGN_32BYTES(uint8_t DMA_BUFFER aRxBuffer[BUFFER_ALIGNED_SIZE]);

uint8_t Spi_PollTillIdle(SPI_HandleTypeDef *handle, uint8_t *);
uint8_t Spi_ParseResponse(const uint8_t *, uint8_t, uint8_t *);
void SPI1_DMA_RX_IRQHandler(void);
void SPI1_DMA_TX_IRQHandler(void);

static uint8_t m_NotifyTransferComplete(SPI_HandleTypeDef *hspi);
static uint8_t m_NotifyTransferError(SPI_HandleTypeDef *hspi);
static uint8_t m_NotifyTransferIssued(SPI_HandleTypeDef *hspi);

static inline uint8_t is_in_dma_nocache(const void *addr, size_t len)
{
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end   = start + len - 1U;

    return (uint8_t)((start >= (uintptr_t)&__dma_buffers_start)
                     && (end < (uintptr_t)&__dma_buffers_end));
}

uint8_t m_GetRccInstance(SPI_HandleTypeDef *handle, uint64_t *instance)
{
    uint8_t res = 0;

    if (NULL == handle->Instance)
    {
        res = 1;
        return res;
    }

    if (handle->Instance == SPI1)
    {
        *instance = RCC_PERIPHCLK_SPI1;
    }
    else if (handle->Instance == SPI2)
    {
        *instance = RCC_PERIPHCLK_SPI2;
    }
    else if (handle->Instance == SPI3)
    {
        *instance = RCC_PERIPHCLK_SPI3;
    }
    else if (handle->Instance == SPI4)
    {
        *instance = RCC_PERIPHCLK_SPI4;
    }
    else if (handle->Instance == SPI5)
    {
        *instance = RCC_PERIPHCLK_SPI5;
    }
    else if (handle->Instance == SPI6)
    {
        *instance = RCC_PERIPHCLK_SPI6;
    }
    else
    {
        res = 2;
    }

    return res;
}

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *handle)
{
    HAL_StatusTypeDef res;
    uint32_t SpiClock;
    uint64_t RccInstance;

    if (NULL == handle)
    {
        Spi_ErrorHandlerHook();
    }

    handle->Instance = SPI1;
    res              = m_GetRccInstance(handle, &RccInstance);

    if (HAL_OK != res)
    {
        Spi_ErrorHandlerHook();
    }

    SpiClock = HAL_RCCEx_GetPeriphCLKFreq(RccInstance);

    pSpiHandle1 = handle;

    handle->Init.Mode = SPI_MODE_MASTER;
    // handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    handle->Init.BaudRatePrescaler =
        SpiUtils_ComputePrescaler(SpiClock, 4000000U);
    handle->Init.Direction = SPI_DIRECTION_2LINES;
    handle->Init.CLKPhase =
        SPI_PHASE_1EDGE; // CPHA = 0: Data captured on the rising edge
    handle->Init.CLKPolarity =
        SPI_POLARITY_LOW; // CPOL = 0: Clock is low when idle
    handle->Init.DataSize       = SPI_DATASIZE_8BIT;
    handle->Init.FirstBit       = SPI_FIRSTBIT_MSB;
    handle->Init.TIMode         = SPI_TIMODE_DISABLE;
    handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle->Init.CRCPolynomial  = 7;
    handle->Init.CRCLength      = SPI_CRC_LENGTH_8BIT;
    handle->Init.NSS            = SPI_NSS_SOFT;
    handle->Init.NSSPMode       = SPI_NSS_PULSE_DISABLE;
    handle->Init.MasterKeepIOState =
        SPI_MASTER_KEEP_IO_STATE_ENABLE; /* Recommended setting to avoid glitches */
    res = HAL_SPI_Init(handle);
    return res;
}

static inline bool m_IsCacheAligned(const void *ptr, size_t size)
{
    uintptr_t addr = (uintptr_t)ptr;
    return ((addr & CACHE_LINE_MASK) == 0) && ((size & CACHE_LINE_MASK) == 0);
}

uint8_t Spi_Send(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len)
{
    uint8_t RetVal;

    Spi_Lock(0);

    memcpy(aTxBuffer, buffer, len);

    if (!is_in_dma_nocache((void *)aTxBuffer, len))
    {
        SCB_CleanDCache_by_Addr((uint32_t *)aTxBuffer, len);
    }

    RetVal = HAL_SPI_Transmit_DMA(handle, aTxBuffer, len);

    if (RetVal == HAL_BUSY)
    {
    }
    else if (RetVal != HAL_OK)
    {
        /* Transfer error in transmission process */
        Spi_ErrorHandlerHook();
        Spi_Unlock(0);
        return HAL_ERROR;
    }

    if (m_NotifyTransferIssued(handle) != 0)
    {
        // Handle timeout
        Spi_Unlock(0);
        return HAL_TIMEOUT;
    }

    if (!is_in_dma_nocache((void *)aTxBuffer, len))
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)aTxBuffer, len);
    }

    Spi_Unlock(0);

    Spi_NotifyRxData(handle, 0);

    return RetVal;
}

uint8_t Spi_SendReceiveMsg(
    SPI_HandleTypeDef *handle,
    const uint8_t *pTxBuffer,
    uint8_t *pRxBuffer,
    uint16_t TxBytes
)
{
    uint8_t RetVal;

    Spi_Lock(0);

    if (!is_in_dma_nocache((void *)pTxBuffer, TxBytes))
    {
        SCB_CleanDCache_by_Addr((uint32_t *)pTxBuffer, TxBytes);
    }

    RetVal = HAL_SPI_TransmitReceive_DMA(handle, pTxBuffer, aRxBuffer, TxBytes);

    if (RetVal == HAL_BUSY)
    {
    }
    else if (RetVal != HAL_OK)
    {
        /* Transfer error in transmission process */
        Spi_ErrorHandlerHook();
        Spi_Unlock(0);
        return HAL_ERROR;
    }

    if (m_NotifyTransferIssued(handle) != 0)
    {
        // Handle timeout
        Spi_Unlock(0);
        Spi_NotifyRxData(handle, 1);
        return HAL_TIMEOUT;
    }

    if (!is_in_dma_nocache((void *)aRxBuffer, TxBytes))
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)aRxBuffer, TxBytes);
    }

    memcpy(pRxBuffer, aRxBuffer, TxBytes);

    Spi_Unlock(0);

    Spi_NotifyRxData(handle, 0);

    return RetVal;
}

uint8_t Spi_Receive(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len)
{
    uint8_t RetVal;

    Spi_Lock(0);

    RetVal = HAL_SPI_Receive_DMA(handle, aRxBuffer, len);

    if (RetVal == HAL_BUSY)
    {
    }
    else if (RetVal != HAL_OK)
    {
        /* Transfer error in transmission process */
        Spi_ErrorHandlerHook();
        Spi_Unlock(0);
        return HAL_ERROR;
    }

    if (m_NotifyTransferIssued(handle) != 0)
    {
        // Handle timeout
        Spi_Unlock(0);
        return HAL_TIMEOUT;
    }

    if (!is_in_dma_nocache((void *)aRxBuffer, len))
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)aRxBuffer, len);
    }

    memcpy(buffer, aRxBuffer, len);

    Spi_Unlock(0);

    Spi_NotifyRxData(handle, 0);

    return RetVal;
}

uint8_t
Spi_ParseResponse(const uint8_t *buffer, uint8_t length, uint8_t *response)
{
    uint8_t RetVal = 1;

    *response = 0xFF;

    for (int i = 0; i < length; i++)
    {
        if (0xFF != buffer[i])
        {
            *response = buffer[i];
            RetVal    = 0;
        }
    }

    return RetVal;
}

uint8_t Spi_readByte(SPI_HandleTypeDef *handle, uint8_t *pResponse)
{
    uint8_t RetVal;
    const uint16_t Bytes = 1;

    RetVal = Spi_SendReceiveMsg(
        handle,
        (uint8_t *)aRxSpiDummy,
        (uint8_t *)aRxBuffer,
        Bytes
    );

    Spi_ParseResponse(aRxBuffer, (uint8_t)Bytes, pResponse);

    return RetVal;
}

uint8_t Spi_writByte(SPI_HandleTypeDef *handle, const uint8_t *data)
{
    uint8_t RetVal;
    uint8_t RespDummy = 0U;

    (void)RespDummy;

    RetVal = Spi_SendReceiveMsg(
        handle,
        (uint8_t const *)data,
        (uint8_t *)&RespDummy,
        1U
    );

    return RetVal;
}

uint8_t Spi_PollForResponse(SPI_HandleTypeDef *handle, uint8_t *pResponse)
{
    uint8_t NoResponseReceived;
    uint8_t RetVal;
    uint8_t counter;
    const uint8_t RetryCount = 10;

    counter            = 0;
    NoResponseReceived = 1;

    do
    {
        Spi_readByte(handle, pResponse);

        if (0xFF != *pResponse)
        {
            NoResponseReceived = 0;
        }

        counter++;
    } while (NoResponseReceived && (RetryCount > counter));

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

uint8_t Spi_PollTillIdle(SPI_HandleTypeDef *handle, uint8_t *pResponse)
{
    uint8_t NoResponseReceived;
    uint8_t RetVal;
    uint8_t counter;
    const uint8_t RetryCount = 10;

    counter            = 0;
    NoResponseReceived = 1;

    do
    {
        Spi_readByte(handle, pResponse);

        if (0xFF == *pResponse)
        {
            NoResponseReceived = 0;
        }

        counter++;
    } while (NoResponseReceived && (RetryCount > counter));

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

uint8_t Spi_goHighSpeed(SPI_HandleTypeDef *handle)
{
    uint8_t res;
    volatile uint32_t SpiClock;
    uint64_t RccInstance;

    res = 0U;

    __disable_irq();

    if (HAL_SPI_Abort(handle) != HAL_OK)
    {
        Spi_ErrorHandlerHook();
    }

    res = m_GetRccInstance(handle, &RccInstance);

    if (HAL_OK != res)
    {
        return res;
    }

    SpiClock = HAL_RCCEx_GetPeriphCLKFreq(RccInstance);

    handle->Init.BaudRatePrescaler =
        SpiUtils_ComputePrescaler(SpiClock, 25000000U);
    res = HAL_SPI_Init(handle);
    __enable_irq();

    if (res != HAL_OK)
    {
        Spi_ErrorHandlerHook();
    }

    return res;
}

uint8_t m_NotifyTransferIssued(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    res         = Spi_NotifyTransferIssued(hspi);
    return res;
}

uint8_t m_NotifyTransferComplete(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    Spi_NotifyTransferComplete(hspi);
    return res;
}

uint8_t m_NotifyTransferError(SPI_HandleTypeDef *hspi)
{
    uint8_t res = 0;
    Spi_NotifyTransferError(hspi);
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

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    m_NotifyTransferComplete(hspi);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
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
void __attribute__((weak)) Spi_ErrorHandlerHook(void)
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
