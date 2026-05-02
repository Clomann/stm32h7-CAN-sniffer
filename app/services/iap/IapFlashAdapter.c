#include "IapFlashAdapter.h"

#include "flash.h"

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
    (void)ctx;
    return IapFlashAdapter_MapFlashStatus(Flash_Write(addr, src, len));
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

void IapFlashAdapter_InitOps(IapWriterStorageOpsType *storage_ops)
{
    if (storage_ops == NULL)
    {
        return;
    }

    storage_ops->ctx          = NULL;
    storage_ops->erase        = IapFlashAdapter_Erase;
    storage_ops->write        = IapFlashAdapter_Write;
    storage_ops->read         = IapFlashAdapter_Read;
    storage_ops->get_property = IapFlashAdapter_GetProperty;
}
