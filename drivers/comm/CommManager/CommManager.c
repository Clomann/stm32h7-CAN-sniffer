/*
 * CommManager.c
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#include "CommManager.h"
#include "CommFactory.h"

comm_status_t CommManager_Init(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx)
{
    const CommFactoryEntry *e = CommFactory_Find(drv->protocol);
    if (!e) 
    {
        /* unknown protocol */
        return COMM_ERROR;
    }

    return e->create(drv, cfg, cfg_size, tx, rx);
}
