#include <string.h>

#include "Spi_Cmds.h"

/* Private define ------------------------------------------------------------*/
enum
{
    TRANSFER_WAIT,
    TRANSFER_COMPLETE,
    TRANSFER_ERROR
};

uint8_t aRxSpiDummy[1024U];
uint8_t aRxSpiSink[1024U];

volatile int Spi_Init_return_value         = 0;
volatile int Spi_Init_call_count           = 0;
volatile int Spi_SendReceiveMsg_call_count = 0;
uint8_t Spi_SendReceiveMsg_last_tx[4U * 512U];
uint8_t Spi_SendReceiveMsg_last_rx[4U * 512U];
size_t Spi_SendReceiveMsg_last_len = 0;

volatile int Spi_Receive_call_count     = 0;
size_t Spi_Receive_last_len             = 0;
uint8_t Spi_Receive_last_rx_buffer[512] = {0};

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *handle)
{
    (void)handle;
    Spi_Init_call_count++;
    return Spi_Init_return_value;
}

HAL_StatusTypeDef Spi_SendReceiveMsg(
    SPI_HandleTypeDef *handle,
    const uint8_t *pTxBuffer,
    uint8_t *pRxBuffer,
    uint16_t Size
)
{
    (void)handle;

    Spi_SendReceiveMsg_call_count++;
    Spi_SendReceiveMsg_last_len = Size;

    if (pTxBuffer && Size <= sizeof(Spi_SendReceiveMsg_last_tx))
    {
        memcpy(Spi_SendReceiveMsg_last_tx, pTxBuffer, Size);
    }

    if (pRxBuffer && Size <= sizeof(Spi_SendReceiveMsg_last_rx))
    {
        memcpy(pRxBuffer, Spi_SendReceiveMsg_last_rx, Size);
    }

    return HAL_OK;
}

uint8_t Spi_Receive(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len)
{
    Spi_Receive_call_count++;
    Spi_Receive_last_len = len;

    if (buffer != NULL && len <= sizeof(Spi_Receive_last_rx_buffer))
    {
        memcpy(Spi_Receive_last_rx_buffer, buffer, len);
        memset(buffer, 0xAA, len);
    }

    (void)handle;
    return HAL_OK;
}

uint8_t Spi_Send(SPI_HandleTypeDef *handle, uint8_t *buffer, uint16_t len)
{
    (void)handle;
    (void)buffer;
    (void)len;
    return HAL_OK;
}
