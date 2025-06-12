/*
 * CommManager.h
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#pragma once
#pragma once

#include <string.h>

#include "CommTypes.h"
#include "CommHandle.h"
#include "CommMessages.h"

typedef comm_status_t (*RegisterMessageFunction)(void* message, void* protocolSpecific, uint32_t *msgId);

/* config type forward declarations: 
    need to be completed by the respective drivers 
*/
typedef struct FdcanConfigType FdcanConfigType;

typedef struct CommDriverConfigType {
    CommConfigType config;
    CommDeviceNumberType devNbr;
    void *driver;
} CommDriverConfigType;

typedef struct CommDriver {
	CommInterface *interface;
	CommDriverConfigType *config;
    void *instance;
	CommProtocolType protocol;
	CommDriverStatesType state;
	RingBuffer *RxFrameBuffer;
	RingBuffer *TxFrameBuffer;
} CommDriver;

comm_status_t CommManager_Init(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    RingBuffer *tx,
    RingBuffer *rx);
