/*
 * CommMessages.h
 *
 *  Created on: 10.10.2024
 *      Author: Clemens
 */

#ifndef CM7_DRIVERS_COMM_INC_COMMMESSAGES_H_
#define CM7_DRIVERS_COMM_INC_COMMMESSAGES_H_

#include "CommTypes.h"

typedef struct {
	Message msgBase;
    uint32_t can_id;
    bool isExtendedId;
    uint8_t frame_type;
    uint8_t msgMarker;
} FDCAN_Message;

#endif /* CM7_DRIVERS_COMM_INC_COMMMESSAGES_H_ */
