#include <string.h>

#include "Spi_Cmds.h"

/* Private define ------------------------------------------------------------*/
enum {
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};


volatile int Spi_Init_return_value = 0;
volatile int Spi_Init_call_count = 0;
volatile int Spi_SendReceiveMsg_call_count = 0;
uint8_t Spi_SendReceiveMsg_last_tx[4U * 512U];
uint8_t Spi_SendReceiveMsg_last_rx[4U * 512U];
size_t Spi_SendReceiveMsg_last_len = 0;

HAL_StatusTypeDef Spi_Init(SPI_HandleTypeDef *handle)
{
    (void)handle;
    Spi_Init_call_count++;
    return Spi_Init_return_value;
}

HAL_StatusTypeDef Spi_SendReceiveMsg(SPI_HandleTypeDef * handle, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint16_t Size)
{
    (void)handle;
    
    Spi_SendReceiveMsg_call_count++;
    Spi_SendReceiveMsg_last_len = Size;
    
    // Copy TX data for verification in tests
    if (pTxBuffer && Size <= sizeof(Spi_SendReceiveMsg_last_tx)) {
        memcpy(Spi_SendReceiveMsg_last_tx, pTxBuffer, Size);
    }
    
    // Simulate received data if needed
    if (pRxBuffer && Size <= sizeof(Spi_SendReceiveMsg_last_rx)) {
        memcpy(pRxBuffer, Spi_SendReceiveMsg_last_rx, Size);
    }
    
    return HAL_OK;
}