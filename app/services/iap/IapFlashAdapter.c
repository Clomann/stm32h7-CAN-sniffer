#include "IapFlashAdapter.h"

#include "IapWriter.h"
#include "flash.h"
#include <stdint.h>
#include <string.h>

static IapWriterStorageStatusType
IapFlashAdapter_MapFlashStatus(FlashStatusType flash_status)
{
    switch (flash_status)
    {
    case FLASH_E_OK:
        return IAP_WRITER_STORAGE_E_OK;
    case FLASH_E_PARAM:
        return IAP_WRITER_STORAGE_E_PARAM;
    case FLASH_E_RANGE:
        return IAP_WRITER_STORAGE_E_RANGE;
    case FLASH_E_ALIGN:
        return IAP_WRITER_STORAGE_E_ALIGN;
    case FLASH_E_VERIFY:
        return IAP_WRITER_STORAGE_E_VERIFY;
    case FLASH_E_BUSY:
    case FLASH_E_PROTECT:
    case FLASH_E_HW:
    default:
        return IAP_WRITER_STORAGE_E_IO;
    }
}

static IapWriterStorageStatusType
IapFlashAdapter_Erase(void *ctx, uint32_t addr, size_t len)
{
    (void)ctx;
    return IapFlashAdapter_MapFlashStatus(Flash_Erase(addr, len));
}

static IapWriterStorageStatusType
IapFlashAdapter_Write(void *ctx, uint32_t addr, const void *src, size_t len)
{
    uint32_t updated_len     = 0;
    uint32_t addr_offset     = 0;
    uintptr_t source_pointer = 0;
    FlashInfoType info;
    IapFlashAdapterContextType *context;
    IapWriterStorageStatusType res = IAP_WRITER_STORAGE_E_OK;

    if (NULL == src || NULL == ctx)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if (Flash_GetInfo(&info) != FLASH_E_OK)
    {
        return IAP_WRITER_STORAGE_E_IO;
    }

    context = (IapFlashAdapterContextType *)ctx;

    if (0U == context->max_chunk || 0 == info.prog_size_min)
    {
        return IAP_WRITER_STORAGE_E_VERIFY;
    }

    source_pointer = (uintptr_t)src;

    if (context->src_alignment <= 0
        || sizeof(context->bounce_raw) % context->src_alignment != 0)
    {
        return IAP_WRITER_STORAGE_E_ALIGN;
    }

    updated_len = len;
    addr_offset = 0;

    while (updated_len > 0)
    {
        uint32_t copy_len = 0;

        if (updated_len > context->max_chunk)
        {
            copy_len = context->max_chunk;
        }
        else if (updated_len >= info.prog_size_min)
        {
            copy_len = (updated_len / info.prog_size_min) * info.prog_size_min;
        }

        if (0U == copy_len)
        {
            return IAP_WRITER_STORAGE_E_ALIGN;
        }

        memcpy(
            context->bounce_raw,
            (uint8_t *)(source_pointer + addr_offset),
            copy_len
        );

        res = IapFlashAdapter_MapFlashStatus(
            Flash_Write(addr + addr_offset, context->bounce_raw, copy_len)
        );

        if (IAP_WRITER_STORAGE_E_OK != res)
        {
            return res;
        }

        addr_offset += copy_len;

        if (updated_len >= copy_len)
        {
            updated_len -= copy_len;
        }
        else
        {
            updated_len = 0;
        }
    }

    return res;
}

static IapWriterStorageStatusType
IapFlashAdapter_Read(void *ctx, uint32_t addr, void *dst, size_t len)
{
    (void)ctx;
    return IapFlashAdapter_MapFlashStatus(Flash_Read(addr, dst, len));
}

static IapWriterStorageStatusType IapFlashAdapter_GetProperty(
    void *ctx,
    IapWriterStoragePropertyIdType property_id,
    void *value,
    size_t value_len
)
{
    FlashInfoType info;
    uint32_t out_value = 0u;

    (void)ctx;

    if (value == NULL || value_len != sizeof(uint32_t))
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if (Flash_GetInfo(&info) != FLASH_E_OK)
    {
        return IAP_WRITER_STORAGE_E_IO;
    }

    switch (property_id)
    {
    case IAP_WRITER_STORAGE_PROP_ERASE_SIZE:
        out_value = info.erase_size_min;
        break;
    case IAP_WRITER_STORAGE_PROP_PROG_SIZE:
        out_value = info.prog_size_min;
        break;
    case IAP_WRITER_STORAGE_PROP_ALIGNMET:
        out_value = info.source_alignment;
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

IapWriterStorageStatusType IapFlashAdapter_InitOps(
    IapWriterStorageOpsType *storage_ops,
    IapFlashAdapterContextType *ctx
)
{
    FlashInfoType info;

    if (storage_ops == NULL || ctx == NULL)
    {
        return IAP_WRITER_STORAGE_E_PARAM;
    }

    if (Flash_GetInfo(&info) != FLASH_E_OK)
    {
        return IAP_WRITER_STORAGE_E_IO;
    }

    if (0U == info.prog_size_min || 0 == info.source_alignment)
    {
        return IAP_WRITER_STORAGE_E_VERIFY;
    }

    if ((uintptr_t)ctx->bounce_raw % info.source_alignment != 0)
    {
        return IAP_WRITER_STORAGE_E_ALIGN;
    }

    ctx->src_alignment = info.source_alignment;

    ctx->max_chunk = (IAP_FLASH_ADAPTER_BOUNCE_SIZE / info.prog_size_min)
                     * info.prog_size_min;

    if (ctx->max_chunk == 0U)
    {
        return IAP_WRITER_STORAGE_E_VERIFY;
    }

    memset(ctx->bounce_raw, 0x0, sizeof(ctx->bounce_raw));

    storage_ops->ctx          = (void *)ctx;
    storage_ops->erase        = IapFlashAdapter_Erase;
    storage_ops->write        = IapFlashAdapter_Write;
    storage_ops->read         = IapFlashAdapter_Read;
    storage_ops->get_property = IapFlashAdapter_GetProperty;

    return IAP_WRITER_STORAGE_E_OK;
}
