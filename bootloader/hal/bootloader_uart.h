#pragma once

#include <stdint.h>
#include <stddef.h>

int32_t Bootloader_Uart_Init(void);

int32_t Bootloader_Uart_Write(const uint8_t *data, size_t len);

void Bootloader_Uart_Deinit(void);
