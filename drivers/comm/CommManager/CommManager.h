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

typedef struct CommDriver {
	CommInterface *interface;
	void *config;
    void *instance;
	CommProtocolType protocol;
	uint8_t configNbr;
	CommDriverStatesType state;
	RingBuffer *RxFrameBuffer;
	RingBuffer *TxFrameBuffer;
} CommDriver;

/* config type forward declarations: 
    need to be completed by the respective drivers 
*/
typedef struct FdcanConfigType FdcanConfigType;

typedef struct CommDriverConfigType {
    CommConfigType config;
    union {
        FdcanConfigType *fdcan;
    };
} CommDriverConfigType;

comm_status_t CommManager_Init(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    RingBuffer *tx,
    RingBuffer *rx);
