/**
 * @file CommFactory.h
 * @brief Provides the Comm modules abstraction layers to
 *        hook in driver implementations.
 */

 #pragma once

#include "CommTypes.h"
#include "CommManager.h"        /* CommDriver definition  */

/* GCC/Clang syntax – adjust for your tool-chain            */
#define COMM_FACTORY_USED_ATTR  __attribute__(( used))
#define COMM_FACTORY_SECTION  __attribute__((section(".comm_factory"), used))

#define COMM_REGISTER_DRIVER(proto, fn)                          \
        static const CommFactoryEntry __comm_factory_##proto     \
        COMM_FACTORY_SECTION = { (proto), (fn) }

typedef comm_status_t (*Comm_DriverCreate)(
        CommDriver          *driver,
        driver_cfg_t         cfg,
        RingBuffer          *rxBuf);

typedef struct {
        driver_protocol_t    protocol;   /* enum key                */
        Comm_DriverCreate    create;     /* constructor for driver  */
} CommFactoryEntry;

const CommFactoryEntry *CommFactory_Find(driver_protocol_t key);
