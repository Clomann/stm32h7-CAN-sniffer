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

#define FDCAN_STATUS_OK     0U
typedef uint8_t FdcanStatusType;

typedef struct {
    uint16_t brp;   /* Clock prescaler. */
    uint8_t sjw;    /* Synchronization jump width. */
    uint8_t tseg1;  /* Number of time quanta to use for propagation segment + segment 1. */
    uint8_t tseg2;  /* Number of time quanta to use for segment 2. */
} FdcanBitTimingType;

typedef struct {
    driver_cfg_t config;
    // Other FDCAN-specific fields
} FDCANHandle;

typedef struct {
    FdcanBitTimingType * bittiming;
    int bitrate;            // CAN bitrate (e.g., 500 kbps)
    int mode;               // CAN mode (normal, loopback, etc.)
    int auto_retransmit;     // Enable/disable auto retransmission
} FdcanConfigType;

typedef struct {
    FdcanStatusType status;
    uint32_t rxBufferFillLevel;
    uint32_t txBufferFillLevel;
    uint32_t rxBufferLength;
    uint32_t txBufferLength;
    FDCAN_ClassicFrame * rxBuffer;
    FDCAN_ClassicFrame * txBuffer;
} FdcanDataType;

typedef struct {
    FdcanConfigType const * cfg;
    FdcanDataType data;
} FdcanDeviceType;

extern const CommInterface FDCAN_Interface;

comm_status_t FDCAN_CreateDriver(CommDriver *, driver_cfg_t, RingBuffer *);
comm_status_t FDCAN_Init(FdcanDeviceType *dev, FdcanConfigType *cfg, uint8_t *rxBuf, uint32_t rxLen, uint8_t *txBuf, uint32_t txLen);
comm_status_t FDCAN_Send(const void*);
comm_status_t FDCAN_Read(void*, uint8_t, uint32_t);

#endif /* COMM_FDCAN_H_ */
