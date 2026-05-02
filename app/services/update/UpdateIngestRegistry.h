/**
 * @file UpdateIngestRegistry.h
 * @brief Registry for update-ingest callbacks used by upload sources.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t UpdateIngestStatusType;

#define UPDATE_INGEST_E_OK          ((UpdateIngestStatusType)0u)
#define UPDATE_INGEST_E_PARAM       ((UpdateIngestStatusType)1u)
#define UPDATE_INGEST_E_STATE       ((UpdateIngestStatusType)2u)
#define UPDATE_INGEST_E_UNAVAILABLE ((UpdateIngestStatusType)3u)
#define UPDATE_INGEST_E_SEQUENCE    ((UpdateIngestStatusType)4u)
#define UPDATE_INGEST_E_RANGE       ((UpdateIngestStatusType)5u)
#define UPDATE_INGEST_E_ALIGN       ((UpdateIngestStatusType)6u)
#define UPDATE_INGEST_E_BACKEND     ((UpdateIngestStatusType)7u)

/**
 * @brief Callback invoked when a new image upload starts.
 *
 * @param ctx        Opaque implementation context.
 * @param image_size Total image size in bytes.
 *
 * @return Update ingest status code.
 */
typedef UpdateIngestStatusType (*UpdateIngestBeginFn)(
    void *ctx,
    size_t image_size
);

/**
 * @brief Callback invoked for one upload chunk.
 *
 * @param ctx    Opaque implementation context.
 * @param offset Chunk offset relative to image start.
 * @param data   Chunk data pointer.
 * @param len    Chunk size in bytes.
 *
 * @return Update ingest status code.
 */
typedef UpdateIngestStatusType (*UpdateIngestWriteChunkFn)(
    void *ctx,
    uint32_t offset,
    const uint8_t *data,
    size_t len
);

/**
 * @brief Callback invoked when upload is complete.
 *
 * @param ctx Opaque implementation context.
 *
 * @return Update ingest status code.
 */
typedef UpdateIngestStatusType (*UpdateIngestFinalizeFn)(void *ctx);

/**
 * @brief Callback invoked when upload must be aborted.
 *
 * @param ctx Opaque implementation context.
 *
 * @return Update ingest status code.
 */
typedef UpdateIngestStatusType (*UpdateIngestAbortFn)(void *ctx);

typedef struct
{
    UpdateIngestBeginFn begin;
    UpdateIngestWriteChunkFn write_chunk;
    UpdateIngestFinalizeFn finalize;
    UpdateIngestAbortFn abort;
} UpdateIngestVTableType;

typedef struct
{
    const UpdateIngestVTableType *vtable;
    void *ctx;
} UpdateIngestBindingType;

/**
 * @brief Register the active update-ingest binding.
 *
 * @param vtable Callback table to register.
 * @param ctx    Opaque context passed to callbacks.
 *
 * @return UPDATE_INGEST_E_OK on success, error code otherwise.
 */
UpdateIngestStatusType
UpdateIngestRegistry_Register(const UpdateIngestVTableType *vtable, void *ctx);

/**
 * @brief Get the currently registered update-ingest binding.
 *
 * @param binding Output binding structure.
 *
 * @return UPDATE_INGEST_E_OK when a binding is available, error otherwise.
 */
UpdateIngestStatusType UpdateIngestRegistry_Get(UpdateIngestBindingType *binding
);

/**
 * @brief Clear the active update-ingest binding.
 */
void UpdateIngestRegistry_Clear(void);
