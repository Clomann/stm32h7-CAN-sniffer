/*
 * CommManager.c
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#include "CommManager.h"
#include "fdcan.h"

comm_status_t CommManager_Init(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    RingBuffer *tx,
    RingBuffer *rx)
{
    const CommFactoryEntry *e = CommFactory_Find(drv->protocol);
    if (!e) 
    {
        /* unknown protocol */
        return COMM_ERROR;
    }

    return e->create(drv, cfg, cfg_size, tx, rx);
}


