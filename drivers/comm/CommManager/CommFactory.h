/**
 * @file CommFactory.h
 * @brief Provides the Comm modules abstraction layers to
 *        hook in driver implementations.
 */

#pragma once

#include "CommManager.h"        /* CommDriver definition  */

/* GCC/Clang syntax – adjust for your tool-chain            */
#define COMM_FACTORY_USED_ATTR  __attribute__(( used))
#define COMM_FACTORY_SECTION  __attribute__((section(".comm_factory"), used))

#define COMM_REGISTER_DRIVER(proto, fn)                          \
    static const CommFactoryEntry __comm_factory_##proto     \
    COMM_FACTORY_SECTION = { (proto), (fn) }

typedef comm_status_t (*Comm_DriverCreate)(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx);

typedef struct {
    CommProtocolType    protocol;   /* enum key                */
    Comm_DriverCreate    create;     /* constructor for driver  */
} CommFactoryEntry;

const CommFactoryEntry *CommFactory_Find(CommProtocolType key);
