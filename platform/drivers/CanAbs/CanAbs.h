#pragma once

#include "stm32h7xx_hal.h"
#include "CommManager.h"
#include "fdcan.h"

#define CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ 1

typedef struct
{
    uint32_t protocol_status;
    uint32_t error_counter;
    uint32_t rx_fifo0_status;
    uint32_t interrupt_flags;
    uint32_t protocol_status_latched;
    uint32_t error_counter_latched;
    uint32_t rx_fifo0_status_latched;
    uint32_t interrupt_flags_latched;
    uint64_t rx_fifo0_lost_events;
    uint64_t protocol_error_events;
    uint64_t error_warning_events;
    uint64_t error_passive_events;
    uint64_t bus_off_events;
} CanAbsFdcanHealthStatsType;

comm_status_t CanAbs_Init_Can1(uint32_t baudrate);
comm_status_t CanAbs_Send_Can1(FDCAN_Message *msg);
comm_status_t CanAbs_Receive_Can1(FDCAN_ClassicFrameType *frame);
comm_status_t CanAbs_Start_Can1(void);
comm_status_t CanAbs_Stop_Can1(void);
comm_status_t CanAbs_SetBaudrate_Can1(uint32_t baudrate);
comm_status_t CanAbs_SetMode_Can1(uint32_t mode);
comm_status_t CanAbs_IsStateOff_Can1(bool *isOff);
uint8_t CanAbs_GetRxHighWater_Can1(uint32_t *frames);
uint8_t CanAbs_GetHwRxFifoHighWater_Can1(uint32_t *frames);
uint8_t CanAbs_GetStaticTxReplayStats_Can1(FdcanStaticTxReplayStatsType *stats);
uint8_t CanAbs_GetFdcanHealth_Can1(CanAbsFdcanHealthStatsType *stats);

comm_status_t CanAbs_Init_Can2(uint32_t baudrate);
comm_status_t CanAbs_Send_Can2(FDCAN_Message *msg);
comm_status_t CanAbs_Receive_Can2(FDCAN_ClassicFrameType *frame);
comm_status_t CanAbs_Start_Can2(void);
comm_status_t CanAbs_Stop_Can2(void);
comm_status_t CanAbs_SetBaudrate_Can2(uint32_t baudrate);
comm_status_t CanAbs_SetMode_Can2(uint32_t mode);
comm_status_t CanAbs_IsStateOff_Can2(bool *isOff);
uint8_t CanAbs_GetRxHighWater_Can2(uint32_t *frames);
uint8_t CanAbs_GetHwRxFifoHighWater_Can2(uint32_t *frames);
uint8_t CanAbs_GetStaticTxReplayStats_Can2(FdcanStaticTxReplayStatsType *stats);
uint8_t CanAbs_GetFdcanHealth_Can2(CanAbsFdcanHealthStatsType *stats);

uint32_t CanAbs_GetRxBufferCapacity(void);
uint32_t CanAbs_GetHwRxFifoCapacity(void);
void CanAbs_ResetFdcanHealth(void);

void CanAbs_ErrorHandler(void);

/**
 * @brief Drain pending Rx frames from all CAN peripherals and notify consumer.
 */
void CanAbs_Drain(void);

/**
  * @brief  Create a Tx CAN message with standard ID.
  * @param  msg: pointer to an FDCAN_Message structure that is the message
  *              to be craeted.
  */
comm_status_t CanAbs_CreateMessage_Standard(
    FDCAN_Message *msg,
    uint32_t id,
    uint8_t *data,
    uint32_t length
);

void CANABS_CheckIsrPollPeriod(uint64_t timestamp, uint64_t timerPeriod);

uint64_t CANABS_ConvertCountToTimestampHook(uint32_t cnt);

/**
  * @brief  Called by the driver on every rx frame.
  * @note This function runs in the ISR context. 
  */
void CanAbs_RxNotificationCallback(void);
