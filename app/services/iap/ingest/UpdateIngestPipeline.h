/**
 * @file UpdateIngestPipeline.h
 * @brief Streaming ingest pipeline for byte-stream update sources.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "UpdateIngestRegistry.h"

typedef UpdateIngestStatusType UpdateIngestPipelineStatusType;

#define UPDATE_INGEST_PIPELINE_E_OK          UPDATE_INGEST_E_OK
#define UPDATE_INGEST_PIPELINE_E_PARAM       UPDATE_INGEST_E_PARAM
#define UPDATE_INGEST_PIPELINE_E_STATE       UPDATE_INGEST_E_STATE
#define UPDATE_INGEST_PIPELINE_E_UNAVAILABLE UPDATE_INGEST_E_UNAVAILABLE
#define UPDATE_INGEST_PIPELINE_E_SEQUENCE    UPDATE_INGEST_E_SEQUENCE
#define UPDATE_INGEST_PIPELINE_E_RANGE       UPDATE_INGEST_E_RANGE
#define UPDATE_INGEST_PIPELINE_E_ALIGN       UPDATE_INGEST_E_ALIGN
#define UPDATE_INGEST_PIPELINE_E_BACKEND     UPDATE_INGEST_E_BACKEND

/**
 * @brief Runtime context for one streamed ingest session.
 */
typedef struct
{
    UpdateIngestBindingType binding; /**< Active ingest binding. */
    uint32_t expected_size; /**< Expected total payload size in bytes. */
    uint32_t received_size; /**< Number of bytes already forwarded. */
    uint8_t active; /**< Session active flag (1 active, 0 inactive). */
} UpdateIngestPipelineContextType;

/**
 * @brief Start an ingest session and forward begin to the active ingest binding.
 *
 * @param context      Pipeline context.
 * @param content_size Expected total payload size from source layer.
 *
 * @return Pipeline status code.
 */
UpdateIngestPipelineStatusType UpdateIngestPipeline_Begin(
    UpdateIngestPipelineContextType *context,
    uint32_t content_size
);

/**
 * @brief Forward a contiguous payload chunk.
 *
 * @param context Pipeline context.
 * @param data    Chunk payload.
 * @param len     Chunk length in bytes.
 *
 * @return Pipeline status code.
 */
UpdateIngestPipelineStatusType UpdateIngestPipeline_Push(
    UpdateIngestPipelineContextType *context,
    const uint8_t *data,
    size_t len
);

/**
 * @brief Finalize the active session.
 *
 * If received bytes do not match expected content size, the pipeline aborts
 * the ingest session and returns state error.
 *
 * @param context Pipeline context.
 *
 * @return Pipeline status code.
 */
UpdateIngestPipelineStatusType
UpdateIngestPipeline_Finish(UpdateIngestPipelineContextType *context);

/**
 * @brief Abort active session and reset pipeline context.
 *
 * @param context Pipeline context.
 *
 * @return Pipeline status code.
 */
UpdateIngestPipelineStatusType
UpdateIngestPipeline_Abort(UpdateIngestPipelineContextType *context);
