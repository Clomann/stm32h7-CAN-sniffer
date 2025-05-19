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

typedef enum {
    COMM_SUCCESS,
    COMM_ERROR,
    COMM_TIMEOUT
} comm_status_t;

typedef enum {
	DRIVER_FDCAN,
	DRIVER_CAN,
    DRIVER_USART,
    DRIVER_SPI,
	DRIVER_I2C,
	DRIVER_ETHERNET,
	DRIVER_BLUETOOTH
} CommProtocolType;

typedef enum {
	DRIVER_CFG0,
	DRIVER_CFG1,
	DRIVER_CFG2,
	DRIVER_CFGn
} CommConfigType;

typedef enum {
    DRIVER_MSGDIRECTION_RX,
	DRIVER_MSGDIRECTION_TX
} driver_msgdir_t;

typedef struct {
	CommProtocolType protocol;
	driver_msgdir_t dir;
    uint32_t length;
    uint8_t* payload;
    bool isMmultiframe;
    void* protocol_data;  // Pointer to protocol-specific data
} Message;

typedef struct {
	uint32_t id;
	uint16_t timestamp;
	uint16_t dlc;
	uint8_t data[8];
} FDCAN_ClassicFrame;

#endif /* CM7_DRIVERS_COMM_COMMTYPES_H_ */
