#include "UpdateIngestPipeline.h"

#include <limits.h>
#include <string.h>

static void UpdateIngestPipeline_Reset(UpdateIngestPipelineContextType *context)
{
    if (context == NULL)
    {
        return;
    }

    (void)memset(context, 0, sizeof(*context));
}

UpdateIngestPipelineStatusType UpdateIngestPipeline_Begin(
    UpdateIngestPipelineContextType *context,
    uint32_t content_size
)
{
    UpdateIngestPipelineStatusType status = UPDATE_INGEST_PIPELINE_E_OK;

    if (context == NULL || content_size == 0u)
    {
        return UPDATE_INGEST_PIPELINE_E_PARAM;
    }

    UpdateIngestPipeline_Reset(context);

    status = UpdateIngestRegistry_Get(&context->binding);
    if (status != UPDATE_INGEST_PIPELINE_E_OK)
    {
        UpdateIngestPipeline_Reset(context);
        return status;
    }

    status = context->binding.vtable->begin(context->binding.ctx, content_size);
    if (status != UPDATE_INGEST_PIPELINE_E_OK)
    {
        UpdateIngestPipeline_Reset(context);
        return status;
    }

    context->expected_size = content_size;
    context->received_size = 0u;
    context->active        = 1u;
    return UPDATE_INGEST_PIPELINE_E_OK;
}

UpdateIngestPipelineStatusType UpdateIngestPipeline_Push(
    UpdateIngestPipelineContextType *context,
    const uint8_t *data,
    size_t len
)
{
    const uint32_t len_u32                = (uint32_t)len;
    UpdateIngestPipelineStatusType status = UPDATE_INGEST_PIPELINE_E_OK;

    if (context == NULL || context->active == 0u || data == NULL || len == 0u
        || len > UINT32_MAX)
    {
        return UPDATE_INGEST_PIPELINE_E_PARAM;
    }

    status = context->binding.vtable->write_chunk(
        context->binding.ctx,
        context->received_size,
        data,
        len
    );
    if (status != UPDATE_INGEST_PIPELINE_E_OK)
    {
        return status;
    }

    context->received_size += len_u32;
    return UPDATE_INGEST_PIPELINE_E_OK;
}

UpdateIngestPipelineStatusType
UpdateIngestPipeline_Finish(UpdateIngestPipelineContextType *context)
{
    UpdateIngestPipelineStatusType status = UPDATE_INGEST_PIPELINE_E_OK;

    if (context == NULL || context->active == 0u)
    {
        return UPDATE_INGEST_PIPELINE_E_STATE;
    }

    if (context->received_size != context->expected_size)
    {
        (void)context->binding.vtable->abort(context->binding.ctx);
        UpdateIngestPipeline_Reset(context);
        return UPDATE_INGEST_PIPELINE_E_STATE;
    }

    status = context->binding.vtable->finalize(context->binding.ctx);
    UpdateIngestPipeline_Reset(context);
    return status;
}

UpdateIngestPipelineStatusType
UpdateIngestPipeline_Abort(UpdateIngestPipelineContextType *context)
{
    UpdateIngestPipelineStatusType status = UPDATE_INGEST_PIPELINE_E_OK;

    if (context == NULL || context->active == 0u)
    {
        return UPDATE_INGEST_PIPELINE_E_STATE;
    }

    status = context->binding.vtable->abort(context->binding.ctx);
    UpdateIngestPipeline_Reset(context);
    return status;
}
