/**
 * @file IapFlashAdapter.h
 * @brief Flash-backed adapter for IapWriter storage callbacks.
 */
#pragma once

#include "IapWriter.h"

#define IAP_FLASH_ADAPTER_BOUNCE_SIZE ((uint32_t)1024U)

typedef struct
{
    uint32_t src_alignment;
    uint32_t max_chunk;
    uint8_t bounce_raw[IAP_FLASH_ADAPTER_BOUNCE_SIZE]
        __attribute__((aligned(32)));
} IapFlashAdapterContextType;

/**
 * @brief Initialize a storage-ops table with flash-backed callbacks.
 *
 * @param storage_ops Output table to initialize.
 */
IapWriterStorageStatusType IapFlashAdapter_InitOps(
    IapWriterStorageOpsType *storage_ops,
    IapFlashAdapterContextType *ctx
);
