/*
 * CommTypes.h
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#ifndef CM7_DRIVERS_COMM_COMMTYPES_H_
#define CM7_DRIVERS_COMM_COMMTYPES_H_

#include <stdint.h>
#include <stdbool.h>

// DLC field (bits 0-3): 4 bits
#define FDCAN_DLC_POS  0
#define FDCAN_DLC_MASK 0x000F

// Data Length field (bits 4-10): 7 bits
#define FDCAN_DATA_LEN_POS  4
#define FDCAN_DATA_LEN_MASK 0x07F0

// FDF flag (bit 11): FD Format
#define FDCAN_FDF_POS  11
#define FDCAN_FDF_MASK 0x0800

// BRS flag (bit 12): Bit Rate Switch
#define FDCAN_BRS_POS  12
#define FDCAN_BRS_MASK 0x1000

// ESI flag (bit 13): Error State Indicator
#define FDCAN_ESI_POS  13
#define FDCAN_ESI_MASK 0x2000

// IDE flag (bit 14): Identifier Extension (0=Standard 11-bit, 1=Extended 29-bit)
#define FDCAN_IDE_POS  14
#define FDCAN_IDE_MASK 0x4000

typedef uint16_t DlcDlFlagsType;

// --- GET macros ---
#define FDCAN_GET_DLC(frame)                                                   \
    (((frame)->dlc_dl_flags & FDCAN_DLC_MASK) >> FDCAN_DLC_POS)
#define FDCAN_GET_DATA_LEN(frame)                                              \
    (((frame)->dlc_dl_flags & FDCAN_DATA_LEN_MASK) >> FDCAN_DATA_LEN_POS)
#define FDCAN_GET_FDF(frame)                                                   \
    (((frame)->dlc_dl_flags & FDCAN_FDF_MASK) >> FDCAN_FDF_POS)
#define FDCAN_GET_BRS(frame)                                                   \
    (((frame)->dlc_dl_flags & FDCAN_BRS_MASK) >> FDCAN_BRS_POS)
#define FDCAN_GET_ESI(frame)                                                   \
    (((frame)->dlc_dl_flags & FDCAN_ESI_MASK) >> FDCAN_ESI_POS)
#define FDCAN_GET_IDE(frame)                                                   \
    (((frame)->dlc_dl_flags & FDCAN_IDE_MASK) >> FDCAN_IDE_POS)

// --- SET macros ---
#define FDCAN_SET_DLC(frame, val)                                              \
    ((frame)->dlc_dl_flags = ((frame)->dlc_dl_flags & ~FDCAN_DLC_MASK)         \
                             | (((val) << FDCAN_DLC_POS) & FDCAN_DLC_MASK))

#define FDCAN_SET_DATA_LEN(frame, val)                                         \
    ((frame)->dlc_dl_flags =                                                   \
         ((frame)->dlc_dl_flags & ~FDCAN_DATA_LEN_MASK)                        \
         | (((val) << FDCAN_DATA_LEN_POS) & FDCAN_DATA_LEN_MASK))

#define FDCAN_SET_FDF(frame, val)                                              \
    ((frame)->dlc_dl_flags = ((frame)->dlc_dl_flags & ~FDCAN_FDF_MASK)         \
                             | (((val) << FDCAN_FDF_POS) & FDCAN_FDF_MASK))

#define FDCAN_SET_BRS(frame, val)                                              \
    ((frame)->dlc_dl_flags = ((frame)->dlc_dl_flags & ~FDCAN_BRS_MASK)         \
                             | (((val) << FDCAN_BRS_POS) & FDCAN_BRS_MASK))

#define FDCAN_SET_ESI(frame, val)                                              \
    ((frame)->dlc_dl_flags = ((frame)->dlc_dl_flags & ~FDCAN_ESI_MASK)         \
                             | (((val) << FDCAN_ESI_POS) & FDCAN_ESI_MASK))

#define FDCAN_SET_IDE(frame, val)                                              \
    ((frame)->dlc_dl_flags = ((frame)->dlc_dl_flags & ~FDCAN_IDE_MASK)         \
                             | (((val) << FDCAN_IDE_POS) & FDCAN_IDE_MASK))

// --- FLAG macros (for setting/clearing flags) ---
#define FDCAN_SET_FLAG(frame, flag_mask) ((frame)->dlc_dl_flags |= (flag_mask))
#define FDCAN_CLEAR_FLAG(frame, flag_mask)                                     \
    ((frame)->dlc_dl_flags &= ~(flag_mask))

typedef enum
{
    COMM_SUCCESS,
    COMM_ERROR,
    COMM_TIMEOUT,
    COMM_INVALID_PARAMETER,
    COMM_INVALID_STATE,
    COMM_NULL_POINTER,
    COMM_NO_RESSOURCES,
    COMM_TX_FULL,
    COMM_RX_FULL,
    COMM_NO_TX_SLOT,
    COMM_NO_RX_SLOT
} comm_status_t;

typedef enum
{
    DRIVER_FDCAN,
    DRIVER_CAN,
    DRIVER_USART,
    DRIVER_SPI,
    DRIVER_I2C,
    DRIVER_ETHERNET,
    DRIVER_BLUETOOTH
} CommProtocolType;

typedef enum
{
    DRIVER_CFG0,
    DRIVER_CFG1,
    DRIVER_CFG2,
    DRIVER_CFGn
} CommConfigType;

typedef enum
{
    COMM_DEVICE_NUMBER_1 = 1U,
    COMM_DEVICE_NUMBER_2,
    COMM_DEVICE_NUMBERn,
} CommDeviceNumberType;

typedef enum
{
    DRIVER_STATE_UNINITIALIZED = 0,
    DRIVER_STATE_INITIALIZED,
    DRIVER_STATE_STOPPED,
    DRIVER_STATE_STARTED,
    DRIVER_STATE_OFF,
} CommDriverStatesType;

typedef enum
{
    DRIVER_MSGDIRECTION_RX,
    DRIVER_MSGDIRECTION_TX
} driver_msgdir_t;

typedef struct
{
    CommProtocolType protocol;
    driver_msgdir_t dir;
    uint32_t length;
    const uint8_t *payload;
    bool isMmultiframe;
    void *protocol_data; // Pointer to protocol-specific data
} Message;

typedef struct
{
    uint32_t id;
    uint32_t timestamp;
    uint8_t channel;
    uint16_t dlc_dl_flags; // Packed: dlc + data_length + flags
    uint8_t data[8];
} FDCAN_ClassicFrameType;

typedef struct
{
    uint32_t id;
    uint32_t timestamp;
    uint8_t channel;
    DlcDlFlagsType dlc_dl_flags; // Packed: dlc + data_length + flags
    uint8_t data[64];
} FDCAN_FdcanFrameType;

#endif /* CM7_DRIVERS_COMM_COMMTYPES_H_ */
