#include "RamFlashSim.h"

#include <string.h>

static bool RamFlashSim_RangeToOffset(
    const RamFlashSimContextType *context,
    uint32_t addr,
    size_t len,
    size_t *offset
)
{
    uint32_t off_u32 = 0u;
    if (!TestFlashMemory_BoundsOk(&context->memory, addr, len, &off_u32))
    {
        return false;
    }

    if (offset != NULL)
    {
        *offset = (size_t)off_u32;
    }

    return true;
}

static IapWriterStorageStatusType
RamFlashSim_Erase(void *ctx, uint32_t addr, size_t len)
{
    RamFlashSimContextType *context = (RamFlashSimContextType *)ctx;
    size_t offset                   = 0u;

    if (context == NULL || context->erase_size == 0u || len == 0u)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if ((addr % context->erase_size) != 0u || (len % context->erase_size) != 0u)
    {
        return IAP_WRITER_STORAGE_E_ALIGN;
    }

    if (!RamFlashSim_RangeToOffset(context, addr, len, &offset))
    {
        return IAP_WRITER_STORAGE_E_RANGE;
    }

    if (context->fail_next_erase)
    {
        context->fail_next_erase = false;
        return IAP_WRITER_STORAGE_E_IO;
    }

    memset(
        TestFlashMemory_GetPtr(&context->memory, (uint32_t)offset),
        0xFF,
        len
    );
    return IAP_WRITER_STORAGE_E_OK;
}

static IapWriterStorageStatusType
RamFlashSim_Write(void *ctx, uint32_t addr, const void *src, size_t len)
{
    RamFlashSimContextType *context = (RamFlashSimContextType *)ctx;
    const uint8_t *src_u8           = (const uint8_t *)src;
    size_t offset                   = 0u;

    if (context == NULL || src_u8 == NULL || context->prog_size == 0u
        || len == 0u)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if ((addr % context->prog_size) != 0u || (len % context->prog_size) != 0u)
    {
        return IAP_WRITER_STORAGE_E_ALIGN;
    }

    if (!RamFlashSim_RangeToOffset(context, addr, len, &offset))
    {
        return IAP_WRITER_STORAGE_E_RANGE;
    }

    if (context->fail_next_write)
    {
        context->fail_next_write = false;
        return IAP_WRITER_STORAGE_E_IO;
    }

    for (size_t i = 0; i < len; i++)
    {
        const uint8_t old_val =
            TestFlashMemory_GetConstPtr(&context->memory, (uint32_t)offset)[i];
        const uint8_t new_val = src_u8[i];
        if ((old_val & new_val) != new_val)
        {
            return IAP_WRITER_STORAGE_E_VERIFY;
        }
    }

    for (size_t i = 0; i < len; i++)
    {
        TestFlashMemory_GetPtr(&context->memory, (uint32_t)offset)[i] &=
            src_u8[i];
    }

    return IAP_WRITER_STORAGE_E_OK;
}

static IapWriterStorageStatusType
RamFlashSim_Read(void *ctx, uint32_t addr, void *dst, size_t len)
{
    RamFlashSimContextType *context = (RamFlashSimContextType *)ctx;
    size_t offset                   = 0u;

    if (context == NULL || dst == NULL || len == 0u)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if (!RamFlashSim_RangeToOffset(context, addr, len, &offset))
    {
        return IAP_WRITER_STORAGE_E_RANGE;
    }

    if (context->fail_next_read)
    {
        context->fail_next_read = false;
        return IAP_WRITER_STORAGE_E_IO;
    }

    memcpy(
        dst,
        TestFlashMemory_GetConstPtr(&context->memory, (uint32_t)offset),
        len
    );
    return IAP_WRITER_STORAGE_E_OK;
}

static IapWriterStorageStatusType RamFlashSim_GetProperty(
    void *ctx,
    IapWriterStoragePropertyIdType property_id,
    void *value,
    size_t value_len
)
{
    RamFlashSimContextType *context = (RamFlashSimContextType *)ctx;
    uint32_t out_value              = 0u;

    if (context == NULL || value == NULL || value_len != sizeof(uint32_t))
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    switch (property_id)
    {
    case IAP_WRITER_STORAGE_PROP_ERASE_SIZE:
        out_value = context->erase_size;
        break;
    case IAP_WRITER_STORAGE_PROP_PROG_SIZE:
        out_value = context->prog_size;
        break;
    default:
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if (out_value == 0u)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    *(uint32_t *)value = out_value;
    return IAP_WRITER_STORAGE_E_OK;
}

void RamFlashSim_Init(
    RamFlashSimContextType *context,
    uint8_t *storage,
    size_t storage_size,
    uint32_t base_addr,
    uint32_t erase_size,
    uint32_t prog_size
)
{
    if (context == NULL || storage == NULL || storage_size == 0u)
    {
        return;
    }

    TestFlashMemory_Init(&context->memory, storage, storage_size, base_addr);
    context->erase_size      = erase_size;
    context->prog_size       = prog_size;
    context->fail_next_erase = false;
    context->fail_next_write = false;
    context->fail_next_read  = false;

    RamFlashSim_Reset(context);
}

void RamFlashSim_Reset(RamFlashSimContextType *context)
{
    if (context == NULL)
    {
        return;
    }

    TestFlashMemory_Reset(&context->memory, 0xFF);
    context->fail_next_erase = false;
    context->fail_next_write = false;
    context->fail_next_read  = false;
}

IapWriterStorageOpsType
RamFlashSim_GetStorageOps(RamFlashSimContextType *context)
{
    IapWriterStorageOpsType ops;

    ops.ctx          = context;
    ops.erase        = RamFlashSim_Erase;
    ops.write        = RamFlashSim_Write;
    ops.read         = RamFlashSim_Read;
    ops.get_property = RamFlashSim_GetProperty;

    return ops;
}

void RamFlashSim_Fill(
    RamFlashSimContextType *context,
    uint32_t addr,
    uint8_t value,
    size_t len
)
{
    size_t offset = 0u;

    if (!RamFlashSim_RangeToOffset(context, addr, len, &offset))
    {
        return;
    }

    memset(
        TestFlashMemory_GetPtr(&context->memory, (uint32_t)offset),
        value,
        len
    );
}

IapWriterStorageStatusType RamFlashSim_CopyOut(
    RamFlashSimContextType *context,
    uint32_t addr,
    void *dst,
    size_t len
)
{
    return RamFlashSim_Read(context, addr, dst, len);
}

void RamFlashSim_CorruptByte(
    RamFlashSimContextType *context,
    uint32_t addr,
    uint8_t value
)
{
    size_t offset = 0u;

    if (!RamFlashSim_RangeToOffset(context, addr, 1u, &offset))
    {
        return;
    }

    TestFlashMemory_GetPtr(&context->memory, (uint32_t)offset)[0] = value;
}
