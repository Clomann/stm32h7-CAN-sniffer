#pragma once
#include "CanAbs.h"

/**
 * @brief Receives and buffers CAN frames from ISR
 * 
 * Accumulates frames into Page. Sends to CanFrameBuffer when full.
 * 
 * @param[in] frame Received CAN frame
 * @note ISR context only
 */
void fdcan_msg_port_receive(FDCAN_ClassicFrameType *frame);

/**
 * @brief Flushes partial page from task context
 * 
 * Sends buffered frames to CanFrameBuffer. Call after disabling interrupts
 * to avoid losing frames when stopping reception.
 * 
 * @note Task context only - uses critical section for thread safety
 */
void fdcan_msg_port_flush(void);

void fdcan_msg_port_init(void);

size_t fdcan_msg_port_read(FDCAN_ClassicFrameType **dst, uint32_t milliSeconds);
