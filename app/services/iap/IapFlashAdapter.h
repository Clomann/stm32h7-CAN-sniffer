/**
 * @file IapFlashAdapter.h
 * @brief Flash-backed adapter for IapWriter storage callbacks.
 */
#pragma once

#include "IapWriter.h"

/**
 * @brief Initialize a storage-ops table with flash-backed callbacks.
 *
 * @param storage_ops Output table to initialize.
 */
void IapFlashAdapter_InitOps(IapWriterStorageOpsType *storage_ops);
