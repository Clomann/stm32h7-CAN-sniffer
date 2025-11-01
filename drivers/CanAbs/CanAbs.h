#pragma once

#include "stm32h7xx_hal.h"
#include "CommManager.h"

extern volatile uint32_t CanAbs_FrameCount;

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

void  CanAbs_ErrorHandler(void);

/**
  * @brief  Create a Tx CAN message with standard ID.
  * @param  msg: pointer to an FDCAN_Message structure that is the message
  *              to be craeted.
  */
comm_status_t CanAbs_CreateMessage_Standard(
    FDCAN_Message *msg, 
    uint32_t id, 
    uint8_t *data, 
    uint32_t length);

uint64_t CANABS_ConvertCountToTimestampHook(uint32_t cnt);

/**
  * @brief  Called by the driver on every rx frame.
  * @note This function runs in the ISR context. 
  */
void CanAbs_RxNotificationCallback(
    void
);
