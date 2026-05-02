/**
 * @file IapIngestAdapter.h
 * @brief Adapter that exposes IapWriter via UpdateIngest callbacks.
 */
#pragma once

#include "IapWriter.h"
#include "UpdateIngestRegistry.h"

#define IAP_INGEST_ADAPTER_ALIGN_MAX ((uint32_t)64U)

typedef struct
{
    IapWriterContextType *writer;
    uint32_t pending_len;
    uint8_t pending[IAP_INGEST_ADAPTER_ALIGN_MAX] __attribute__((aligned(32)));
} IapIngestAdapterContextType;

/**
 * @brief Initialize ingest-adapter context.
 *
 * @param context Adapter context to initialize.
 * @param writer  Writer instance used by ingress callbacks.
 *
 * @return UPDATE_INGEST_E_OK on success, error code otherwise.
 */
UpdateIngestStatusType IapIngestAdapter_Init(
    IapIngestAdapterContextType *context,
    IapWriterContextType *writer
);

/**
 * @brief Get immutable vtable for registering ingest callbacks.
 *
 * @return Pointer to ingest callback table.
 */
const UpdateIngestVTableType *IapIngestAdapter_GetVTable(void);
