/**
 * @file IapWriter.h
 * @brief Public interface for incremental firmware image writing to the IAP
 *        target slot.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t IapWriterStatusType;

#define IAP_WRITER_E_OK       ((IapWriterStatusType)0u)
#define IAP_WRITER_E_PARAM    ((IapWriterStatusType)1u)
#define IAP_WRITER_E_STATE    ((IapWriterStatusType)2u)
#define IAP_WRITER_E_RANGE    ((IapWriterStatusType)3u)
#define IAP_WRITER_E_ALIGN    ((IapWriterStatusType)4u)
#define IAP_WRITER_E_SEQUENCE ((IapWriterStatusType)5u)
#define IAP_WRITER_E_STORAGE  ((IapWriterStatusType)6u)
#define IAP_WRITER_E_VERIFY   ((IapWriterStatusType)7u)
/* Backward-compatibility alias. */
#define IAP_WRITER_E_FLASH IAP_WRITER_E_STORAGE

/**
 * @brief Storage callback return status for IAP writer.
 */
typedef uint8_t IapWriterStorageStatusType;

#define IAP_WRITER_STORAGE_E_OK     ((IapWriterStorageStatusType)0u)
#define IAP_WRITER_STORAGE_E_PARAM  ((IapWriterStorageStatusType)1u)
#define IAP_WRITER_STORAGE_E_RANGE  ((IapWriterStorageStatusType)2u)
#define IAP_WRITER_STORAGE_E_ALIGN  ((IapWriterStorageStatusType)3u)
#define IAP_WRITER_STORAGE_E_VERIFY ((IapWriterStorageStatusType)4u)
#define IAP_WRITER_STORAGE_E_IO     ((IapWriterStorageStatusType)5u)

/**
 * @brief Storage property identifier type used by get_property callback.
 */
typedef uint16_t IapWriterStoragePropertyIdType;

#define IAP_WRITER_STORAGE_PROP_ERASE_SIZE ((IapWriterStoragePropertyIdType)1u)
#define IAP_WRITER_STORAGE_PROP_PROG_SIZE  ((IapWriterStoragePropertyIdType)2u)

/**
 * @brief Storage erase callback used by IAP writer.
 *
 * @param ctx  Opaque implementation context.
 * @param addr Start address to erase.
 * @param len  Erase length in bytes.
 *
 * @return IAP_WRITER_STORAGE_E_OK on success, backend error code otherwise.
 */
typedef IapWriterStorageStatusType (*IapWriterStorageEraseFn)(
    void *ctx,
    uint32_t addr,
    size_t len
);

/**
 * @brief Storage write callback used by IAP writer.
 *
 * @param ctx  Opaque implementation context.
 * @param addr Destination address.
 * @param src  Source data buffer.
 * @param len  Number of bytes to write.
 *
 * @return IAP_WRITER_STORAGE_E_OK on success, backend error code otherwise.
 */
typedef IapWriterStorageStatusType (*IapWriterStorageWriteFn)(
    void *ctx,
    uint32_t addr,
    const void *src,
    size_t len
);

/**
 * @brief Storage read callback used by IAP writer.
 *
 * @param ctx  Opaque implementation context.
 * @param addr Source address in storage.
 * @param dst  Destination buffer.
 * @param len  Number of bytes to read.
 *
 * @return IAP_WRITER_STORAGE_E_OK on success, backend error code otherwise.
 */
typedef IapWriterStorageStatusType (*IapWriterStorageReadFn)(
    void *ctx,
    uint32_t addr,
    void *dst,
    size_t len
);

/**
 * @brief Storage property query callback used by IAP writer.
 *
 * @param ctx         Opaque implementation context.
 * @param property_id Property identifier.
 * @param value       Output buffer receiving property value.
 * @param value_len   Output buffer size in bytes.
 *
 * @return IAP_WRITER_STORAGE_E_OK on success, backend error code otherwise.
 */
typedef IapWriterStorageStatusType (*IapWriterStorageGetPropertyFn)(
    void *ctx,
    IapWriterStoragePropertyIdType property_id,
    void *value,
    size_t value_len
);

typedef struct
{
    void *ctx;
    IapWriterStorageEraseFn erase;
    IapWriterStorageWriteFn write;
    IapWriterStorageReadFn read;
    IapWriterStorageGetPropertyFn get_property;
} IapWriterStorageOpsType;

typedef struct
{
    uint32_t slot_addr;
    uint32_t slot_size;
    /**
     * @brief Erase granularity in bytes.
     *
     * Set to 0 to resolve from storage ops via
     * `IAP_WRITER_STORAGE_PROP_ERASE_SIZE`.
     */
    uint32_t erase_size;
    /**
     * @brief Program granularity in bytes.
     *
     * Set to 0 to resolve from storage ops via
     * `IAP_WRITER_STORAGE_PROP_PROG_SIZE`.
     */
    uint32_t prog_size;
} IapWriterConfigType;

typedef struct
{
    IapWriterConfigType config;
    IapWriterStorageOpsType storage_ops;
    uint32_t expected_size;
    uint32_t received_size;
    uint32_t payload_hash;
    bool initialized;
    bool active;
    bool finalized;
} IapWriterContextType;

/**
 * @brief Initialize an IAP writer context.
 *
 * @param context     Writer context to initialize.
 * @param config      Slot and alignment configuration.
 * @param storage_ops Storage operation callbacks used by the writer.
 *
 * @return IAP_WRITER_E_OK on success, error code otherwise.
 */
IapWriterStatusType IapWriter_Init(
    IapWriterContextType *context,
    const IapWriterConfigType *config,
    const IapWriterStorageOpsType *storage_ops
);

/**
 * @brief Start a new image write session.
 *
 * @param context    Initialized writer context.
 * @param image_size Expected image size in bytes.
 *
 * @return IAP_WRITER_E_OK on success, error code otherwise.
 */
IapWriterStatusType
IapWriter_Begin(IapWriterContextType *context, size_t image_size);

/**
 * @brief Write one sequential image chunk into the configured slot.
 *
 * @param context Writer context in active state.
 * @param offset  Chunk offset from image start (must be sequential).
 * @param data    Source buffer.
 * @param len     Chunk length in bytes.
 *
 * @return IAP_WRITER_E_OK on success, error code otherwise.
 */
IapWriterStatusType IapWriter_WriteChunk(
    IapWriterContextType *context,
    uint32_t offset,
    const uint8_t *data,
    size_t len
);

/**
 * @brief Finalize current session and verify written content by readback.
 *
 * @param context Writer context in active state.
 *
 * @return IAP_WRITER_E_OK on success, error code otherwise.
 */
IapWriterStatusType IapWriter_FinalizeAndVerify(IapWriterContextType *context);

/**
 * @brief Abort the current session and reset runtime state.
 *
 * @param context Writer context.
 *
 * @return IAP_WRITER_E_OK on success, error code otherwise.
 */
IapWriterStatusType IapWriter_Abort(IapWriterContextType *context);

/**
 * @brief Get expected image size for the current or last session.
 *
 * @param context Writer context.
 *
 * @return Expected size in bytes, or 0 if context is invalid.
 */
uint32_t IapWriter_GetExpectedSize(const IapWriterContextType *context);

/**
 * @brief Get number of bytes accepted so far.
 *
 * @param context Writer context.
 *
 * @return Received size in bytes, or 0 if context is invalid.
 */
uint32_t IapWriter_GetReceivedSize(const IapWriterContextType *context);
