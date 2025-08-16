#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int Spi_Init_return_value;
extern volatile int Spi_Init_call_count;
extern volatile int Spi_SendReceiveMsg_call_count;
extern uint8_t Spi_SendReceiveMsg_last_tx[];
extern uint8_t Spi_SendReceiveMsg_last_rx[];
extern size_t Spi_SendReceiveMsg_last_len;

extern volatile int Spi_Receive_call_count;
extern size_t Spi_Receive_last_len;
extern uint8_t Spi_Receive_last_rx_buffer[512];

void SpiAbs_ErrorHandler(void);

#ifdef __cplusplus
}
#endif
