/*
 * CommHandle.h
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

#ifndef COMM_COMMHANDLE_H_
#define COMM_COMMHANDLE_H_

#include <stdlib.h>
#include "CommTypes.h"
#include "buffers.h"

typedef struct CommDriver CommDriver;

/*! Opaque init funciton that needs to be implemented by each driver.
 * \param[in] cfg_size Allows the dirver sanity-check the config.
 */
typedef comm_status_t (*Comm_Init)(CommDriver *drv);
typedef comm_status_t (*Comm_Deinit)(CommDriver *handle);
typedef comm_status_t (*Comm_Send)(const void *msg);
typedef comm_status_t (*Comm_Read)(void *data, uint8_t length, uint32_t RxFifo0ITs);
typedef comm_status_t (*Comm_Control)(CommDriver *handle, int command, void *argument);
typedef comm_status_t (*Comm_RegisterCallback)(CommDriver *handle, void (*callback)(void *), void *context);
typedef comm_status_t (*Comm_EnableInterrupt)(CommDriver *handle);
typedef comm_status_t (*Comm_DisableInterrupt)(CommDriver *handle);

typedef struct {
    Comm_Init init;
    Comm_Deinit deinit;
    Comm_Send send;
    Comm_Read read;
    Comm_Control control;
    Comm_RegisterCallback register_callback;
    Comm_EnableInterrupt enable_interrupt;
    Comm_DisableInterrupt disable_interrupt;
} CommInterface;

#endif /* COMM_COMMHANDLE_H_ */
