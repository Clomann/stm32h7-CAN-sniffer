#include "IapWriter.h"

#include <string.h>

#define IAP_WRITER_FNV1A_OFFSET_BASIS (2166136261u)
#define IAP_WRITER_FNV1A_PRIME        (16777619u)
#define IAP_WRITER_VERIFY_BUF_SIZE    (256u)

static uint32_t
IapWriter_HashBytes(uint32_t seed, const uint8_t *data, size_t len)
{
    uint32_t hash = seed;

    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= IAP_WRITER_FNV1A_PRIME;
    }

    return hash;
}

static bool IapWriter_IsAligned(uint32_t value, uint32_t align)
{
    return (align != 0u) && ((value % align) == 0u);
}

static bool
IapWriter_RoundUp(uint32_t value, uint32_t alignment, uint32_t *rounded)
{
    if (rounded == NULL || alignment == 0u)
    {
        return false;
    }

    const uint32_t rem = value % alignment;
    if (rem == 0u)
    {
        *rounded = value;
        return true;
    }

    if (value > UINT32_MAX - (alignment - rem))
    {
        return false;
    }

    *rounded = value + (alignment - rem);
    return true;
}

static IapWriterStatusType
IapWriter_MapStorageError(IapWriterStorageStatusType storage_status)
{
    switch (storage_status)
    {
    case IAP_WRITER_STORAGE_E_OK:
        return IAP_WRITER_E_OK;
    case IAP_WRITER_STORAGE_E_ALIGN:
        return IAP_WRITER_E_ALIGN;
    case IAP_WRITER_STORAGE_E_RANGE:
        return IAP_WRITER_E_RANGE;
    case IAP_WRITER_STORAGE_E_VERIFY:
        return IAP_WRITER_E_VERIFY;
    case IAP_WRITER_STORAGE_E_PARAM:
    case IAP_WRITER_STORAGE_E_IO:
    default:
        return IAP_WRITER_E_STORAGE;
    }
}

static IapWriterStatusType IapWriter_GetStoragePropertyU32(
    const IapWriterStorageOpsType *storage_ops,
    IapWriterStoragePropertyIdType property_id,
    uint32_t *value
)
{
    IapWriterStorageStatusType storage_status = IAP_WRITER_STORAGE_E_OK;

    if (storage_ops == NULL || storage_ops->get_property == NULL
        || value == NULL)
    {
        return IAP_WRITER_E_PARAM;
    }

    storage_status = storage_ops->get_property(
        storage_ops->ctx,
        property_id,
        value,
        sizeof(*value)
    );
    if (storage_status != IAP_WRITER_STORAGE_E_OK)
    {
        if (storage_status == IAP_WRITER_STORAGE_E_PARAM)
        {
            return IAP_WRITER_E_PARAM;
        }
        return IapWriter_MapStorageError(storage_status);
    }

    if (*value == 0u)
    {
        return IAP_WRITER_E_PARAM;
    }

    return IAP_WRITER_E_OK;
}

IapWriterStatusType IapWriter_Init(
    IapWriterContextType *context,
    const IapWriterConfigType *config,
    const IapWriterStorageOpsType *storage_ops
)
{
    uint32_t erase_size        = 0u;
    uint32_t prog_size         = 0u;
    IapWriterStatusType status = IAP_WRITER_E_OK;

    if (context == NULL || config == NULL || storage_ops == NULL)
    {
        return IAP_WRITER_E_PARAM;
    }

    if (config->slot_size == 0u)
    {
        return IAP_WRITER_E_PARAM;
    }

    if (storage_ops->erase == NULL || storage_ops->write == NULL
        || storage_ops->read == NULL)
    {
        return IAP_WRITER_E_PARAM;
    }

    erase_size = config->erase_size;
    prog_size  = config->prog_size;

    if (erase_size == 0u)
    {
        status = IapWriter_GetStoragePropertyU32(
            storage_ops,
            IAP_WRITER_STORAGE_PROP_ERASE_SIZE,
            &erase_size
        );
        if (status != IAP_WRITER_E_OK)
        {
            return status;
        }
    }

    if (prog_size == 0u)
    {
        status = IapWriter_GetStoragePropertyU32(
            storage_ops,
            IAP_WRITER_STORAGE_PROP_PROG_SIZE,
            &prog_size
        );
        if (status != IAP_WRITER_E_OK)
        {
            return status;
        }
    }

    if ((config->slot_size % erase_size) != 0u)
    {
        return IAP_WRITER_E_ALIGN;
    }

    if ((erase_size % prog_size) != 0u)
    {
        return IAP_WRITER_E_ALIGN;
    }

    memset(context, 0, sizeof(*context));
    context->config            = *config;
    context->config.erase_size = erase_size;
    context->config.prog_size  = prog_size;
    context->storage_ops       = *storage_ops;
    context->initialized       = true;

    return IAP_WRITER_E_OK;
}

IapWriterStatusType
IapWriter_Begin(IapWriterContextType *context, size_t image_size)
{
    uint32_t erase_len                        = 0u;
    IapWriterStorageStatusType storage_status = IAP_WRITER_STORAGE_E_OK;

    if (context == NULL || !context->initialized)
    {
        return IAP_WRITER_E_PARAM;
    }

    if (context->active)
    {
        return IAP_WRITER_E_STATE;
    }

    if (image_size == 0u || image_size > context->config.slot_size
        || image_size > UINT32_MAX)
    {
        return IAP_WRITER_E_RANGE;
    }

    if (!IapWriter_IsAligned((uint32_t)image_size, context->config.prog_size))
    {
        return IAP_WRITER_E_ALIGN;
    }

    if (!IapWriter_RoundUp(
            (uint32_t)image_size,
            context->config.erase_size,
            &erase_len
        ))
    {
        return IAP_WRITER_E_RANGE;
    }

    if (erase_len > context->config.slot_size)
    {
        return IAP_WRITER_E_RANGE;
    }

    storage_status = context->storage_ops.erase(
        context->storage_ops.ctx,
        context->config.slot_addr,
        erase_len
    );
    if (storage_status != IAP_WRITER_STORAGE_E_OK)
    {
        return IapWriter_MapStorageError(storage_status);
    }

    context->expected_size = (uint32_t)image_size;
    context->received_size = 0u;
    context->payload_hash  = IAP_WRITER_FNV1A_OFFSET_BASIS;
    context->active        = true;
    context->finalized     = false;

    return IAP_WRITER_E_OK;
}

IapWriterStatusType IapWriter_WriteChunk(
    IapWriterContextType *context,
    uint32_t offset,
    const uint8_t *data,
    size_t len
)
{
    const uint32_t len_u32                    = (uint32_t)len;
    IapWriterStorageStatusType storage_status = IAP_WRITER_STORAGE_E_OK;

    if (context == NULL || data == NULL || len == 0u || len > UINT32_MAX)
    {
        return IAP_WRITER_E_PARAM;
    }

    if (!context->initialized || !context->active || context->finalized)
    {
        return IAP_WRITER_E_STATE;
    }

    if (offset != context->received_size)
    {
        return IAP_WRITER_E_SEQUENCE;
    }

    if (!IapWriter_IsAligned(offset, context->config.prog_size)
        || !IapWriter_IsAligned(len_u32, context->config.prog_size))
    {
        return IAP_WRITER_E_ALIGN;
    }

    if (offset > context->expected_size || len_u32 > context->expected_size
        || offset > context->expected_size - len_u32)
    {
        return IAP_WRITER_E_RANGE;
    }

    storage_status = context->storage_ops.write(
        context->storage_ops.ctx,
        context->config.slot_addr + offset,
        data,
        len
    );
    if (storage_status != IAP_WRITER_STORAGE_E_OK)
    {
        return IapWriter_MapStorageError(storage_status);
    }

    context->payload_hash =
        IapWriter_HashBytes(context->payload_hash, data, len);
    context->received_size += len_u32;

    return IAP_WRITER_E_OK;
}

IapWriterStatusType IapWriter_FinalizeAndVerify(IapWriterContextType *context)
{
    uint8_t verify_buf[IAP_WRITER_VERIFY_BUF_SIZE];
    uint32_t verify_hash = IAP_WRITER_FNV1A_OFFSET_BASIS;
    uint32_t offset      = 0u;

    if (context == NULL || !context->initialized)
    {
        return IAP_WRITER_E_PARAM;
    }

    if (!context->active || context->finalized)
    {
        return IAP_WRITER_E_STATE;
    }

    if (context->received_size != context->expected_size)
    {
        return IAP_WRITER_E_STATE;
    }

    while (offset < context->expected_size)
    {
        const uint32_t remaining = context->expected_size - offset;
        const size_t chunk_len   = (remaining > IAP_WRITER_VERIFY_BUF_SIZE)
                                       ? IAP_WRITER_VERIFY_BUF_SIZE
                                       : (size_t)remaining;

        const IapWriterStorageStatusType storage_status =
            context->storage_ops.read(
                context->storage_ops.ctx,
                context->config.slot_addr + offset,
                verify_buf,
                chunk_len
            );
        if (storage_status != IAP_WRITER_STORAGE_E_OK)
        {
            return IapWriter_MapStorageError(storage_status);
        }

        verify_hash = IapWriter_HashBytes(verify_hash, verify_buf, chunk_len);
        offset += (uint32_t)chunk_len;
    }

    if (verify_hash != context->payload_hash)
    {
        return IAP_WRITER_E_VERIFY;
    }

    context->active    = false;
    context->finalized = true;
    return IAP_WRITER_E_OK;
}

IapWriterStatusType IapWriter_Abort(IapWriterContextType *context)
{
    if (context == NULL || !context->initialized)
    {
        return IAP_WRITER_E_PARAM;
    }

    context->active        = false;
    context->finalized     = false;
    context->expected_size = 0u;
    context->received_size = 0u;
    context->payload_hash  = 0u;

    return IAP_WRITER_E_OK;
}

uint32_t IapWriter_GetExpectedSize(const IapWriterContextType *context)
{
    if (context == NULL)
    {
        return 0u;
    }

    return context->expected_size;
}

uint32_t IapWriter_GetReceivedSize(const IapWriterContextType *context)
{
    if (context == NULL)
    {
        return 0u;
    }

    return context->received_size;
}
