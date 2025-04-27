/*
 * CommManager.c
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#include "CommManager.h"
#include "fdcan.h"

comm_status_t comm_manager_init(CommDriver *pDriver, driver_protocol_t protocol, driver_cfg_t config, RingBuffer *pRxBuffer)
{
	comm_status_t RetVal;

	RetVal = COMM_SUCCESS;

	switch (protocol)
	{
		case DRIVER_FDCAN:
			FDCAN_CreateDriver(pDriver, config, pRxBuffer);
			break;
		case DRIVER_CAN:
		case DRIVER_USART:
		case DRIVER_SPI:
		case DRIVER_I2C:
		case DRIVER_ETHERNET:
		default:
			RetVal = COMM_ERROR;
	}

	return RetVal;
}


