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

#ifndef FDCAN_MODE_DEFAULT
#error "FDCAN_MODE_DEFAULT is not defined! You can select: e.g. FDCAN_MODE_NORMAL)" 
#endif 

#define SW_RX_FRAME_BUFFER_SIZE (2U * FDCAN_RAM_RX_ELEMENTS) /* software Rx frame buffer size in number of FDCAN_ClassicFrame elements */
#define SW_TX_FRAME_BUFFER_SIZE (FDCAN_RAM_TX_ELEMENTS)      /* software Tx frame buffer size in number of FDCAN_ClassicFrame elements */

typedef enum {
    CANABS_IOCTL_CMD_SET_BAUDRATE,
    CANABS_IOCTL_CMD_SET_FILTERMASK,
    CANABS_IOCTL_CMD_START,
    CANABS_IOCTL_CMD_STOP,
    CANABS_IOCTL_CMD_SET_MODE,
} FdcanIoctlCmdType;

#define FDCAN_BAUDRATE_250000      250000U
#define FDCAN_BAUDRATE_500000      500000U
#define FDCAN_BAUDRATE_1000000     1000000U
typedef uint32_t FdcanBaudrateType;

#define FDCAN_MODE_1      1U /*<! normal */
#define FDCAN_MODE_2      2U /*<! listen only */
#define FDCAN_MODE_3      3U /*<! off */
typedef uint32_t FdcanModeType;

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
    FDCAN_ClassicFrameType * rxBuffer;
    FDCAN_ClassicFrameType * txBuffer;
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
    uint8_t *tx, 
    uint8_t *rx) COMM_FACTORY_USED_ATTR;

comm_status_t FDCAN_Init(
    CommDriver *dev);

comm_status_t FDCAN_DeInit(
    CommDriver *dev);
    
comm_status_t FDCAN_Send(
    CommDriver *dev,
    const void*);

comm_status_t FDCAN_Read(
    CommDriver *dev,
    void*, 
    uint8_t, 
    uint32_t);

comm_status_t fdcan_get_can(CommDriver *dev, FDCAN_HandleTypeDef **fdcan);

uint64_t FDCAN_GetMostRecentInterruptTimestamp(CommDriver *dev);

/* shims needed to be implemented by the caller */
void FDCAN_ErrorHandler(void);
uint64_t FDCAN_GetTimestampHook(void);
uint64_t FDCAN_GetTimerPeriodHook(void);

#endif /* COMM_FDCAN_H_ */
