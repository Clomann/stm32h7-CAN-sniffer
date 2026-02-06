#include "Spi_Cmds.h"
#include "spi.h"

/* transfer state */
__IO uint32_t wTransferState = TRANSFER_WAIT;

uint8_t  __attribute__((weak)) Spi_NotifyTransferIssued(SPI_HandleTypeDef *hspi)
{
    while (wTransferState == TRANSFER_WAIT)
	{
	}

    // Wait until the SPI is no longer busy
	while (HAL_SPI_GetState(pSpiHandle1) != HAL_SPI_STATE_READY) {}
    return (uint8_t) 0;
}

uint8_t  __attribute__((weak)) Spi_NotifyTransferComplete(SPI_HandleTypeDef *hspi)
{
    wTransferState = TRANSFER_COMPLETE;
    return (uint8_t) 0;
}

uint8_t  __attribute__((weak)) Spi_NotifyTransferError(SPI_HandleTypeDef *hspi)
{
    wTransferState = TRANSFER_ERROR;
    return (uint8_t) 0;
}

__attribute__((weak)) void Spi_Lock(uint8_t id) 
{ 
    ;
}

__attribute__((weak)) void Spi_Unlock(uint8_t id)
{ 
    ;
}
