/*
 * CommManager.h
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#ifndef CM7_DRIVERS_COMM_COMMMANAGER_H_
#define CM7_DRIVERS_COMM_COMMMANAGER_H_

#include <string.h>

#include "CommTypes.h"
#include "CommHandle.h"
#include "CommMessages.h"
#include "buffers.h"

typedef comm_status_t (*RegisterMessageFunction)(void* message, void* protocolSpecific, uint32_t *msgId);

typedef struct CommDriver {
	CommInterface *interface;
	void *config;
	driver_protocol_t protocol;
	uint8_t configNbr;
	uint8_t initialized;
	RingBuffer *RxFrameBuffer;
	RingBuffer *TxFrameBuffer;
} CommDriver;

comm_status_t comm_manager_init(CommDriver *, driver_protocol_t, driver_cfg_t, RingBuffer *pRxBuffer);

#endif /* CM7_DRIVERS_COMM_COMMMANAGER_H_ */
