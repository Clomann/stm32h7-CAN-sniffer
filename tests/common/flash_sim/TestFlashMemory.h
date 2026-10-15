#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *storage;
    size_t size;
    uint32_t base_addr;
} TestFlashMemoryType;

void TestFlashMemory_Init(
    TestFlashMemoryType *memory,
    uint8_t *storage,
    size_t size,
    uint32_t base_addr
);

void TestFlashMemory_Reset(TestFlashMemoryType *memory, uint8_t fill_value);

int TestFlashMemory_BoundsOk(
    const TestFlashMemoryType *memory,
    uintptr_t addr,
    size_t len,
    uint32_t *off_out
);

uint8_t *TestFlashMemory_GetPtr(TestFlashMemoryType *memory, uint32_t off);

const uint8_t *
TestFlashMemory_GetConstPtr(const TestFlashMemoryType *memory, uint32_t off);

void *TestFlashMemory_Memcpy(
    TestFlashMemoryType *memory,
    void *dst,
    const void *src,
    size_t len
);
