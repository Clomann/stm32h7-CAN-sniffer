#include "Iap.h"

#include "flash.h"
#include "IapFlashAdapter.h"
#include "IapWriter.h"
#include "ingest/UpdateIngestRegistry.h"
#include "ingest/UpdateIngestPipeline.h"
#include "ingest/IapIngestAdapter.h"
#include <stddef.h>
#include <stdint.h>

#include "IapCfg.h"

#define WRITER_CONFIG                                                          \
    {                                                                          \
        .slot_addr  = APPLICATION_SECONDARY_START_ADDRESS,                     \
        .slot_size  = APPLICATION_SECONDARY_SIZE,                              \
        .erase_size = 0u,                                                      \
        .prog_size  = 0u,                                                      \
    };

static IapWriterStorageOpsType IapWriterStorageOperations = {0};
static IapWriterContextType IapWriterContext              = {0};
static IapFlashAdapterContextType IapFlashAdapterContext  = {0};

static IapIngestAdapterContextType IapIngestAdapterContext;
static const UpdateIngestVTableType *UpdateIngestVTable;

#define IAP_VERIFY_STEP_BYTES ((size_t)1024u)

typedef struct
{
    IapVerifyStateType state;
    uint32_t processed;
    uint32_t total;
} IapVerifyRuntimeType;

static IapVerifyRuntimeType IapVerifyRuntime = {
    .state     = IAP_VERIFY_STATE_IDLE,
    .processed = 0u,
    .total     = 0u,
};

IapErrorType Iap_Init(void)
{
    IapErrorType res                          = IAP_E_NOT_OK;
    const IapWriterConfigType IapWriterConfig = WRITER_CONFIG;

    FlashStatusType FlashRes;
    IapWriterStatusType IapWriterRes;
    UpdateIngestStatusType UpdateIngestRes;
    UpdateIngestStatusType UpdateIngestStatusRes;
    IapWriterStorageStatusType IapWriterStorageRes;

    FlashRes = Flash_Init();

    if (FLASH_E_OK == FlashRes)
    {
        res = IAP_E_OK;
    }

    if (IAP_E_OK == res)
    {
        IapWriterStorageRes = IapFlashAdapter_InitOps(
            &IapWriterStorageOperations,
            &IapFlashAdapterContext
        );

        if (IAP_WRITER_STORAGE_E_OK != IapWriterStorageRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    if (IAP_E_OK == res)
    {
        IapWriterRes = IapWriter_Init(
            &IapWriterContext,
            &IapWriterConfig,
            &IapWriterStorageOperations
        );

        if (IAP_WRITER_E_OK != IapWriterRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    if (IAP_E_OK == res)
    {
        UpdateIngestStatusRes =
            IapIngestAdapter_Init(&IapIngestAdapterContext, &IapWriterContext);

        if (IAP_UPDATE_INGEST_E_OK != UpdateIngestStatusRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    if (IAP_E_OK == res)
    {
        UpdateIngestVTable = IapIngestAdapter_GetVTable();

        UpdateIngestRes = UpdateIngestRegistry_Register(
            UpdateIngestVTable,
            (void *)&IapIngestAdapterContext
        );

        if (IAP_UPDATE_INGEST_E_OK != UpdateIngestRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    return res;
}

IapErrorType Iap_DeInit(void)
{
    IapErrorType res = IAP_E_OK;

    UpdateIngestRegistry_Clear();
    Iap_VerifyReset();

    return res;
}

void Iap_VerifyReset(void)
{
    IapVerifyRuntime.state     = IAP_VERIFY_STATE_IDLE;
    IapVerifyRuntime.processed = 0u;
    IapVerifyRuntime.total     = 0u;
}

void Iap_VerifyRequest(void)
{
    if (0u == IapWriter_IsFinalized(&IapWriterContext))
    {
        IapVerifyRuntime.state     = IAP_VERIFY_STATE_ERROR;
        IapVerifyRuntime.processed = 0u;
        IapVerifyRuntime.total = IapWriter_GetExpectedSize(&IapWriterContext);
        return;
    }

    IapVerifyRuntime.state     = IAP_VERIFY_STATE_PENDING;
    IapVerifyRuntime.processed = 0u;
    IapVerifyRuntime.total     = IapWriter_GetExpectedSize(&IapWriterContext);
}

void Iap_VerifyPoll(void)
{
    uint8_t done               = 0u;
    uint32_t processed         = 0u;
    uint32_t total             = 0u;
    IapWriterStatusType status = IAP_WRITER_E_OK;

    if (IapVerifyRuntime.state == IAP_VERIFY_STATE_PENDING)
    {
        if (0u == IapWriter_IsFinalized(&IapWriterContext))
        {
            IapVerifyRuntime.state = IAP_VERIFY_STATE_ERROR;
            return;
        }
        IapVerifyRuntime.state = IAP_VERIFY_STATE_VERIFYING;
    }

    if (IapVerifyRuntime.state != IAP_VERIFY_STATE_VERIFYING)
    {
        return;
    }

    status = IapWriter_VerifyStep(
        &IapWriterContext,
        IAP_VERIFY_STEP_BYTES,
        &processed,
        &total,
        &done
    );
    if (status != IAP_WRITER_E_OK)
    {
        IapVerifyRuntime.state = IAP_VERIFY_STATE_ERROR;
        return;
    }

    IapVerifyRuntime.processed = processed;
    IapVerifyRuntime.total     = total;
    if (done != 0u)
    {
        IapVerifyRuntime.state = IAP_VERIFY_STATE_VERIFIED;
    }
}

void Iap_GetVerifyStatus(
    IapVerifyStateType *state,
    uint32_t *processed,
    uint32_t *total
)
{
    if (state != NULL)
    {
        *state = IapVerifyRuntime.state;
    }
    if (processed != NULL)
    {
        *processed = IapVerifyRuntime.processed;
    }
    if (total != NULL)
    {
        *total = IapVerifyRuntime.total;
    }
}

uint8_t Iap_IsVerified(void)
{
    return (IapVerifyRuntime.state == IAP_VERIFY_STATE_VERIFIED) ? 1u : 0u;
}

IapPrepareStatusType Iap_PrepareUploadSlot(void)
{
    IapWriterStorageStatusType storage_status = IAP_WRITER_STORAGE_E_OK;

    if (!IapWriterContext.initialized)
    {
        return IAP_PREPARE_E_PARAM;
    }

    if (IapWriterContext.active)
    {
        return IAP_PREPARE_E_STATE;
    }

    if (IapWriterContext.storage_ops.erase == NULL)
    {
        return IAP_PREPARE_E_PARAM;
    }

    storage_status = IapWriterContext.storage_ops.erase(
        IapWriterContext.storage_ops.ctx,
        IapWriterContext.config.slot_addr,
        IapWriterContext.config.slot_size
    );
    if (storage_status != IAP_WRITER_STORAGE_E_OK)
    {
        return IAP_PREPARE_E_BACKEND;
    }

    IapWriterContext.slot_prepared = true;
    Iap_VerifyReset();
    return IAP_PREPARE_E_OK;
}
