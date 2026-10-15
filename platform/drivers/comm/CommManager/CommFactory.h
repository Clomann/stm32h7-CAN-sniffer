/**
 * @file CommFactory.h
 * @brief Provides the Comm modules abstraction layers to
 *        hook in driver implementations.
 */

#pragma once

#include "CommManager.h" /* CommDriver definition  */
#include "memory_sections.h"

/* GCC/Clang syntax – adjust for your tool-chain            */
#define COMM_FACTORY_USED_ATTR __attribute__((used))

#if !defined(UNIT_TEST)
#define COMM_REGISTER_DRIVER(proto, fn)                                        \
    static const CommFactoryEntry __comm_factory_##proto COMM_FACTORY_SECTION  \
        COMM_FACTORY_USED_ATTR = {(proto), (fn)}
#else
#define COMM_REGISTER_DRIVER(proto, fn)
#endif

typedef comm_status_t (*Comm_DriverCreate)(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx
);

typedef struct
{
    CommProtocolType protocol; /* enum key                */
    Comm_DriverCreate create; /* constructor for driver  */
} CommFactoryEntry;

const CommFactoryEntry *CommFactory_Find(CommProtocolType key);
