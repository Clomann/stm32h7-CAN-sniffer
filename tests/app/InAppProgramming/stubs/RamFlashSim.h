#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "IapWriter.h"
#include "TestFlashMemory.h"

typedef struct
{
    TestFlashMemoryType memory;
    uint32_t erase_size;
    uint32_t prog_size;
    bool fail_next_erase;
    bool fail_next_write;
    bool fail_next_read;
} RamFlashSimContextType;

void RamFlashSim_Init(
    RamFlashSimContextType *context,
    uint8_t *storage,
    size_t storage_size,
    uint32_t base_addr,
    uint32_t erase_size,
    uint32_t prog_size
);

void RamFlashSim_Reset(RamFlashSimContextType *context);

IapWriterStorageOpsType
RamFlashSim_GetStorageOps(RamFlashSimContextType *context);

void RamFlashSim_Fill(
    RamFlashSimContextType *context,
    uint32_t addr,
    uint8_t value,
    size_t len
);

IapWriterStorageStatusType RamFlashSim_CopyOut(
    RamFlashSimContextType *context,
    uint32_t addr,
    void *dst,
    size_t len
);

void RamFlashSim_CorruptByte(
    RamFlashSimContextType *context,
    uint32_t addr,
    uint8_t value
);
