/*
 * CommManager.c
 *
 *  Created on: 19.09.2024
 *      Author: Clemens
 */

#include "CommManager.h"
#include "CommFactory.h"

comm_status_t comm_manager_init(
    CommDriver *drv,
    driver_protocol_t proto,
    driver_cfg_t cfg,
    RingBuffer *rx)
{
    const CommFactoryEntry *e = CommFactory_Find(proto);
    if (!e)                     /* unknown protocol */
    return COMM_ERROR;

    return e->create(drv, cfg, rx);
}
