/*
 * fdcan.h
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

#ifndef COMM_FDCAN_H_
#define COMM_FDCAN_H_

#include "stm32h7xx.h"
#include "stm32h7xx_hal_fdcan.h"
#include "CommManager.h"
#include "fdcan_cfg.h"

#ifndef FDCAN_MODE
#define FDCAN_MODE FDCAN_MODE_NORMAL 
#endif 
#define MAX_INSTANCES 1

typedef struct {
    driver_cfg_t config;
    // Other FDCAN-specific fields
} FDCANHandle;

typedef struct {
    int bitrate;            // CAN bitrate (e.g., 500 kbps)
    int mode;               // CAN mode (normal, loopback, etc.)
    int auto_retransmit;     // Enable/disable auto retransmission
} FDCAN_Config;

extern const CommInterface FDCAN_Interface;

comm_status_t FDCAN_CreateDriver(CommDriver *, driver_cfg_t, RingBuffer *);
comm_status_t FDCAN_Init(void);
comm_status_t FDCAN_Send(const void*);
comm_status_t FDCAN_Read(void*, uint8_t, uint32_t);

#endif /* COMM_FDCAN_H_ */
