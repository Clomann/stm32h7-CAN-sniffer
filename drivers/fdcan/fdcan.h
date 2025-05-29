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
#include "fdcan_cfg.h"
#include "CommFactory.h"

#ifndef FDCAN_MODE
#define FDCAN_MODE FDCAN_MODE_NORMAL 
#endif 
#define MAX_INSTANCES 1

typedef enum {
    CANABS_IOCTL_CMD_SET_BAUDRATE,
    CANABS_IOCTL_CMD_SET_FILTERMASK,
    CANABS_IOCTL_CMD_START,
    CANABS_IOCTL_CMD_STOP
} FdcanIoctlCmdType;

#define FDCAN_BAUDRATE_250000      250000U
#define FDCAN_BAUDRATE_500000      500000U
#define FDCAN_BAUDRATE_1000000     1000000U
typedef uint32_t FdcanBaudrateType;

#define FDCAN_STATUS_OK     0U
typedef uint8_t FdcanStatusType;

typedef struct {
    uint16_t brp;   /* Clock prescaler. */
    uint8_t sjw;    /* Synchronization jump width. */
    uint8_t tseg1;  /* Number of time quanta to use for propagation segment + segment 1. */
    uint8_t tseg2;  /* Number of time quanta to use for segment 2. */
} FdcanBitTimingType;

typedef struct {
    CommDriverConfigType config;
    // Other FDCAN-specific fields
} FDCANHandle;

typedef struct FdcanConfigType {
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

comm_status_t FDCAN_CreateDriver(
    CommDriver *pDriver, 
    const void *cfg, 
    size_t cfg_size,
    RingBuffer *tx, 
    RingBuffer *rx) COMM_FACTORY_USED_ATTR;

comm_status_t FDCAN_Init(
    CommDriver *dev);

comm_status_t FDCAN_DeInit(
    CommDriver *dev);
    
comm_status_t FDCAN_Send(
    const void*);

comm_status_t FDCAN_Read(
    void*, 
    uint8_t, uint32_t);

void FDCAN_GetMostRecentInterruptTimestamp(
    uint32_t *timestamp);

/* shims needed to be implemented by the caller */
void FDCAN_ErrorHandler(void);
comm_status_t FDCAN_GetTimestamp(
    uint64_t *timestamp);

#endif /* COMM_FDCAN_H_ */
