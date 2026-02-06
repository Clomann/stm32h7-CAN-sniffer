#pragma once

#include <stdint.h>

#define SPI_PORT_USE_LOCKS 1
#define SPI_PORT_USE_HOOKS 1

#define SPI_PORT_USE_DIRECT 1
#if SPI_PORT_USE_DIRECT
#define SPI_PORT_USE_SEMAPHORE 0
#else
#define SPI_PORT_USE_SEMAPHORE 1
#endif

#ifndef MAX_SPI_INSTANCES          /* tune to your MCU */
  #define MAX_SPI_INSTANCES   1
#endif

uint8_t spi_port_freertos_init(void *handle);

/**
 * @param hspi Handle to the SPI instance of type 'SPI_HandleTypeDef *'
 */
void Spi_NotifyRegister(void *hspi);
