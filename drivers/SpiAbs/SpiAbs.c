#include "SpiAbs.h"
#include "Spi_Cmds.h"

static volatile SPI_HandleTypeDef SpiHandle1;

void Spi_ErrorHandler()
{
    SpiAbs_ErrorHandler();
}

void * SpiAbs_GetHandle_Spi1()
{
    return (void*)&SpiHandle1;
}

void __attribute((weak)) SpiAbs_ErrorHandler(void)
{
    while (1) {
    };
}

uint8_t SpiAbs_Init_Spi1()
{
    uint8_t res = 0;
    SPI_Init((SPI_HandleTypeDef *)&SpiHandle1);
    return res;
}