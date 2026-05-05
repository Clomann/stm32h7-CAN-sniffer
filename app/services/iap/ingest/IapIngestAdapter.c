#include "IapIngestAdapter.h"
#include "UpdateIngestRegistry.h"
#include <stdint.h>
#include <string.h>

static UpdateIngestStatusType MapWriterStatus(IapWriterStatusType writer_status)
{
    switch (writer_status)
    {
    case IAP_WRITER_E_OK:
        return IAP_UPDATE_INGEST_E_OK;
    case IAP_WRITER_E_PARAM:
        return IAP_UPDATE_INGEST_E_PARAM;
    case IAP_WRITER_E_STATE:
        return IAP_UPDATE_INGEST_E_STATE;
    case IAP_WRITER_E_SEQUENCE:
        return IAP_UPDATE_INGEST_E_SEQUENCE;
    case IAP_WRITER_E_RANGE:
        return IAP_UPDATE_INGEST_E_RANGE;
    case IAP_WRITER_E_ALIGN:
        return IAP_UPDATE_INGEST_E_ALIGN;
    case IAP_WRITER_E_STORAGE:
    case IAP_WRITER_E_VERIFY:
    default:
        return IAP_UPDATE_INGEST_E_BACKEND;
    }
}

static UpdateIngestStatusType Begin(void *ctx, size_t image_size)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    memset(context->pending, 0x0, sizeof(context->pending));
    context->pending_len = 0;

    return MapWriterStatus(IapWriter_Begin(context->writer, image_size));
}

static UpdateIngestStatusType
CheckOffsetSequence(IapIngestAdapterContextType *ctx, uint32_t offset)
{
    uint32_t writer_off;
    uint32_t expected_off;

    writer_off   = IapWriter_GetReceivedSize(ctx->writer);
    expected_off = writer_off + ctx->pending_len;

    if (offset != expected_off)
    {
        return IAP_UPDATE_INGEST_E_SEQUENCE;
    }

    return IAP_UPDATE_INGEST_E_OK;
}

static UpdateIngestStatusType
WriteChunk(void *ctx, uint32_t offset, const uint8_t *data, size_t len)
{
    uint32_t free_len                    = 0;
    uint32_t remain_len                  = 0;
    uint32_t updated_len                 = 0;
    uint32_t updated_offset              = 0;
    uint32_t prog_size                   = 0;
    UpdateIngestStatusType res           = IAP_UPDATE_INGEST_E_OK;
    uint32_t algined_len                 = 0;
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL || data == NULL
        || NULL == context->writer->storage_ops.get_property)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    if (0 == len)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    res = context->writer->storage_ops.get_property(
        context->writer->storage_ops.ctx,
        IAP_WRITER_STORAGE_PROP_PROG_SIZE,
        &prog_size,
        sizeof(prog_size)
    );

    if (IAP_WRITER_STORAGE_E_OK != res || 0 == prog_size
        || IAP_INGEST_ADAPTER_ALIGN_MAX < prog_size)
    {
        return IAP_UPDATE_INGEST_E_BACKEND;
    }

    res = CheckOffsetSequence(context, offset);
    if (IAP_UPDATE_INGEST_E_OK != res)
    {
        return res;
    }

    updated_len = len;

    // check if pending buffer is partially filled -> fill until full
    if (context->pending_len > 0 && len > 0)
    {
        if (context->pending_len > prog_size)
        {
            return IAP_UPDATE_INGEST_E_STATE;
        }

        free_len = prog_size - context->pending_len;

        if (len >= free_len)
        {
            memcpy(&context->pending[context->pending_len], &data[0], free_len);
            updated_len    = len - free_len;
            updated_offset = offset - context->pending_len;
            context->pending_len += free_len;
        }
        else
        {
            memcpy(&context->pending[context->pending_len], &data[0], len);
            updated_len    = 0;
            updated_offset = offset - context->pending_len;
            context->pending_len += len;
        }
    }

    // write the pendinng buffer that was just filled completely
    if (context->pending_len == prog_size)
    {
        res = MapWriterStatus(IapWriter_WriteChunk(
            context->writer,
            updated_offset,
            context->pending,
            context->pending_len
        ));

        context->pending_len = 0;

        if (IAP_UPDATE_INGEST_E_OK != res)
        {
            return res;
        }
    }

    // handle the main part of the chunk until unaligned remainder
    if (updated_len >= prog_size)
    {
        algined_len = updated_len - updated_len % prog_size;
    }

    // pending buffer is supposed to be empty here
    if (0 == context->pending_len)
    {
        remain_len = updated_len - algined_len;

        if (remain_len > 0)
        {
            memcpy(
                &context->pending[context->pending_len],
                &data[free_len + algined_len],
                remain_len
            );
            context->pending_len += remain_len;
            updated_len = updated_len - remain_len; // should yield be 0 here
        }
    }

    if (algined_len == 0)
    {
        return IAP_UPDATE_INGEST_E_OK;
    }

    // finally actually write main chunk
    return MapWriterStatus(IapWriter_WriteChunk(
        context->writer,
        offset + free_len,
        &data[free_len],
        algined_len
    ));
}

static UpdateIngestStatusType Finalize(void *ctx)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    if (context->pending_len != 0)
    {
        return IAP_UPDATE_INGEST_E_ALIGN;
    }

    return MapWriterStatus(IapWriter_Finalize(context->writer));
}

static UpdateIngestStatusType Abort(void *ctx)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    memset(context->pending, 0x0, sizeof(context->pending));
    context->pending_len = 0;

    return MapWriterStatus(IapWriter_Abort(context->writer));
}

UpdateIngestStatusType IapIngestAdapter_Init(
    IapIngestAdapterContextType *context,
    IapWriterContextType *writer
)
{
    if (context == NULL || writer == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    context->writer = writer;
    return IAP_UPDATE_INGEST_E_OK;
}

const UpdateIngestVTableType *IapIngestAdapter_GetVTable(void)
{
    static const UpdateIngestVTableType vtable = {
        .begin       = Begin,
        .write_chunk = WriteChunk,
        .finalize    = Finalize,
        .abort       = Abort,
    };

    return &vtable;
}
