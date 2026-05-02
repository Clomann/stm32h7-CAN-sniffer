#include "TestFlashMemory.h"

void TestFlashMemory_Init(
    TestFlashMemoryType *memory,
    uint8_t *storage,
    size_t size,
    uint32_t base_addr
)
{
    if (memory == NULL)
    {
        return;
    }

    memory->storage   = storage;
    memory->size      = size;
    memory->base_addr = base_addr;
}

void TestFlashMemory_Reset(TestFlashMemoryType *memory, uint8_t fill_value)
{
    if (memory == NULL || memory->storage == NULL)
    {
        return;
    }

    for (size_t i = 0; i < memory->size; i++)
    {
        memory->storage[i] = fill_value;
    }
}

int TestFlashMemory_BoundsOk(
    const TestFlashMemoryType *memory,
    uintptr_t addr,
    size_t len,
    uint32_t *off_out
)
{
    if (memory == NULL || memory->storage == NULL || len == 0u)
    {
        return 0;
    }

    if (addr < (uintptr_t)memory->base_addr)
    {
        return 0;
    }

    if (addr > UINT32_MAX)
    {
        return 0;
    }

    const uint64_t off = (uint64_t)(addr - (uintptr_t)memory->base_addr);
    if (off + len > (uint64_t)memory->size)
    {
        return 0;
    }

    if (off_out != NULL)
    {
        *off_out = (uint32_t)off;
    }

    return 1;
}

uint8_t *TestFlashMemory_GetPtr(TestFlashMemoryType *memory, uint32_t off)
{
    if (memory == NULL || memory->storage == NULL || off >= memory->size)
    {
        return NULL;
    }

    return &memory->storage[off];
}

const uint8_t *
TestFlashMemory_GetConstPtr(const TestFlashMemoryType *memory, uint32_t off)
{
    if (memory == NULL || memory->storage == NULL || off >= memory->size)
    {
        return NULL;
    }

    return &memory->storage[off];
}

void *TestFlashMemory_Memcpy(
    TestFlashMemoryType *memory,
    void *dst,
    const void *src,
    size_t len
)
{
    uint32_t src_off = 0u;
    uint32_t dst_off = 0u;
    const int src_is_flash =
        TestFlashMemory_BoundsOk(memory, (uintptr_t)src, len, &src_off);
    const int dst_is_flash =
        TestFlashMemory_BoundsOk(memory, (uintptr_t)dst, len, &dst_off);

    if (len == 0u)
    {
        return dst;
    }

    if (src_is_flash && dst_is_flash)
    {
        if (src_off == dst_off)
        {
            return dst;
        }

        if (src_off < dst_off)
        {
            for (size_t i = len; i > 0; --i)
            {
                memory->storage[dst_off + i - 1u] =
                    memory->storage[src_off + i - 1u];
            }
        }
        else
        {
            for (size_t i = 0; i < len; ++i)
            {
                memory->storage[dst_off + i] = memory->storage[src_off + i];
            }
        }

        return dst;
    }

    if (src_is_flash)
    {
        uint8_t *dst_bytes = (uint8_t *)dst;
        for (size_t i = 0; i < len; ++i)
        {
            dst_bytes[i] = memory->storage[src_off + i];
        }
        return dst;
    }

    if (dst_is_flash)
    {
        const uint8_t *src_bytes = (const uint8_t *)src;
        for (size_t i = 0; i < len; ++i)
        {
            memory->storage[dst_off + i] = src_bytes[i];
        }
        return dst;
    }

    uint8_t *dst_bytes       = (uint8_t *)dst;
    const uint8_t *src_bytes = (const uint8_t *)src;
    for (size_t i = 0; i < len; ++i)
    {
        dst_bytes[i] = src_bytes[i];
    }

    return dst;
}
