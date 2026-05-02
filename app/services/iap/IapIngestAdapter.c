#include "IapIngestAdapter.h"

static UpdateIngestStatusType
IapIngestAdapter_MapWriterStatus(IapWriterStatusType writer_status)
{
    switch (writer_status)
    {
    case IAP_WRITER_E_OK:
        return UPDATE_INGEST_E_OK;
    case IAP_WRITER_E_PARAM:
        return UPDATE_INGEST_E_PARAM;
    case IAP_WRITER_E_STATE:
        return UPDATE_INGEST_E_STATE;
    case IAP_WRITER_E_SEQUENCE:
        return UPDATE_INGEST_E_SEQUENCE;
    case IAP_WRITER_E_RANGE:
        return UPDATE_INGEST_E_RANGE;
    case IAP_WRITER_E_ALIGN:
        return UPDATE_INGEST_E_ALIGN;
    case IAP_WRITER_E_STORAGE:
    case IAP_WRITER_E_VERIFY:
    default:
        return UPDATE_INGEST_E_BACKEND;
    }
}

static UpdateIngestStatusType
IapIngestAdapter_Begin(void *ctx, size_t image_size)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return UPDATE_INGEST_E_PARAM;
    }

    return IapIngestAdapter_MapWriterStatus(
        IapWriter_Begin(context->writer, image_size)
    );
}

static UpdateIngestStatusType IapIngestAdapter_WriteChunk(
    void *ctx,
    uint32_t offset,
    const uint8_t *data,
    size_t len
)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return UPDATE_INGEST_E_PARAM;
    }

    return IapIngestAdapter_MapWriterStatus(
        IapWriter_WriteChunk(context->writer, offset, data, len)
    );
}

static UpdateIngestStatusType IapIngestAdapter_Finalize(void *ctx)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return UPDATE_INGEST_E_PARAM;
    }

    return IapIngestAdapter_MapWriterStatus(
        IapWriter_FinalizeAndVerify(context->writer)
    );
}

static UpdateIngestStatusType IapIngestAdapter_Abort(void *ctx)
{
    IapIngestAdapterContextType *context = (IapIngestAdapterContextType *)ctx;

    if (context == NULL || context->writer == NULL)
    {
        return UPDATE_INGEST_E_PARAM;
    }

    return IapIngestAdapter_MapWriterStatus(IapWriter_Abort(context->writer));
}

UpdateIngestStatusType IapIngestAdapter_Init(
    IapIngestAdapterContextType *context,
    IapWriterContextType *writer
)
{
    if (context == NULL || writer == NULL)
    {
        return UPDATE_INGEST_E_PARAM;
    }

    context->writer = writer;
    return UPDATE_INGEST_E_OK;
}

const UpdateIngestVTableType *IapIngestAdapter_GetVTable(void)
{
    static const UpdateIngestVTableType vtable = {
        .begin       = IapIngestAdapter_Begin,
        .write_chunk = IapIngestAdapter_WriteChunk,
        .finalize    = IapIngestAdapter_Finalize,
        .abort       = IapIngestAdapter_Abort,
    };

    return &vtable;
}
