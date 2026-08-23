#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "CommTypes.h"

/* Stub control helpers */
void reset_can_stubs(void);
void set_can_start_result(comm_status_t result);
void set_can_stop_result(comm_status_t result);
void set_can_state_off(bool isOff);
bool get_can_started(void);
bool get_can_stopped(void);

typedef struct
{
    uint32_t requests;
    uint32_t completed;
    uint32_t irqs;
    uint32_t last_completed_mask;
    uint32_t buffer_mask;
    uint32_t next_id_offset;
    uint32_t tx_pending;
    uint32_t tx_occurred;
    uint32_t tx_cancelled;
    uint32_t protocol_status;
    uint32_t error_counter;
    uint32_t reload_errors;
    uint8_t active;
} FdcanStaticTxReplayStatsType;

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

/* CAN abstraction API used by CanLogManager */
comm_status_t CanAbs_Start_Can1(void);
comm_status_t CanAbs_Start_Can2(void);
comm_status_t CanAbs_Stop_Can1(void);
comm_status_t CanAbs_Stop_Can2(void);
comm_status_t CanAbs_IsStateOff_Can1(bool *isOff);
comm_status_t CanAbs_IsStateOff_Can2(bool *isOff);
uint8_t CanAbs_GetRxHighWater_Can1(uint32_t *frames);
uint8_t CanAbs_GetRxHighWater_Can2(uint32_t *frames);
uint32_t CanAbs_GetRxBufferCapacity(void);
uint8_t CanAbs_GetHwRxFifoHighWater_Can1(uint32_t *frames);
uint8_t CanAbs_GetHwRxFifoHighWater_Can2(uint32_t *frames);
uint32_t CanAbs_GetHwRxFifoCapacity(void);
uint8_t CanAbs_GetStaticTxReplayStats_Can1(FdcanStaticTxReplayStatsType *stats);
uint8_t CanAbs_GetStaticTxReplayStats_Can2(FdcanStaticTxReplayStatsType *stats);
uint8_t CanAbs_GetFdcanHealth_Can1(CanAbsFdcanHealthStatsType *stats);
uint8_t CanAbs_GetFdcanHealth_Can2(CanAbsFdcanHealthStatsType *stats);
void CanAbs_ResetFdcanHealth(void);
void CanAbs_Drain(void);
