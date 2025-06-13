#pragma once

#include "stm32h7xx_hal.h"
#include "CommManager.h"

comm_status_t CanAbs_Init_Can1(uint32_t baudrate);
comm_status_t CanAbs_Send_Can1(FDCAN_Message *msg);
comm_status_t CanAbs_Receive_Can1(FDCAN_ClassicFrame *frame);
comm_status_t CanAbs_Start_Can1(void);
comm_status_t CanAbs_Stop_Can1(void);
comm_status_t CanAbs_SetBaudrate_Can1(uint32_t baudrate);
comm_status_t CanAbs_SetMode_Can1(uint32_t mode);
comm_status_t CanAbs_IsStateOff_Can1(bool * isOff);

comm_status_t CanAbs_Init_Can2(uint32_t baudrate);
comm_status_t CanAbs_Send_Can2(FDCAN_Message *msg);
comm_status_t CanAbs_Receive_Can2(FDCAN_ClassicFrame *frame);
comm_status_t CanAbs_Start_Can2(void);
comm_status_t CanAbs_Stop_Can2(void);
comm_status_t CanAbs_SetBaudrate_Can2(uint32_t baudrate);
comm_status_t CanAbs_SetMode_Can2(uint32_t mode);
comm_status_t CanAbs_IsStateOff_Can2(bool * isOff);

comm_status_t fdcan_create_message_1(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_2(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_3(FDCAN_Message*, uint8_t*, uint32_t);
comm_status_t fdcan_create_message_4(FDCAN_Message*, uint8_t*, uint32_t);