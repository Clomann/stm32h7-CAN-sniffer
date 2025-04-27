#pragma once

#include "stm32h7xx_hal.h"
#include "CommManager.h"

int CanAbs_Init(void);
int CanAbs_Send(void);
int CanAbs_Receive(uint32_t *count);

unsigned int fdcan_setup(void);
comm_status_t fdcan_create_message_1(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_2(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_3(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_4(FDCAN_Message*, uint8_t*, uint32_t);