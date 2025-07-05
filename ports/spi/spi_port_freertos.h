#pragma once

#include <stdint.h>

#ifndef MAX_SPI_INSTANCES          /* tune to your MCU */
  #define MAX_SPI_INSTANCES   3
#endif

uint8_t spi_port_freertos_init(void);

/**
 * @param hspi Handle to the SPI instance of type 'SPI_HandleTypeDef *'
 */
void Spi_NotifyRegister(void *hspi);
