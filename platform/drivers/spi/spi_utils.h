#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"

static inline uint32_t
SpiUtils_ComputePrescaler(uint32_t spi_clock, uint64_t max_out_clock)
{
    uint32_t prescaler;
    uint32_t factor;

    factor = (uint32_t)((uint64_t)spi_clock / max_out_clock);

    if (factor <= 2)
    {
        prescaler = SPI_BAUDRATEPRESCALER_2;
    }
    else if (factor <= 4)
    {
        prescaler = SPI_BAUDRATEPRESCALER_4;
    }
    else if (factor <= 8)
    {
        prescaler = SPI_BAUDRATEPRESCALER_8;
    }
    else if (factor <= 16)
    {
        prescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if (factor <= 32)
    {
        prescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if (factor <= 64)
    {
        prescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if (factor <= 128)
    {
        prescaler = SPI_BAUDRATEPRESCALER_128;
    }
    else
    {
        prescaler = SPI_BAUDRATEPRESCALER_256;
    }

    return prescaler;
}
