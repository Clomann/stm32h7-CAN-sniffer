#include "Iap.h"

#include "flash.h"
#include "IapFlashAdapter.h"
#include "IapWriter.h"
#include "ingest/UpdateIngestRegistry.h"
#include "ingest/UpdateIngestPipeline.h"
#include "ingest/IapIngestAdapter.h"
#include <stdint.h>

extern uint8_t __app_secondary_start__;
extern uint8_t __app_secondary_size__;

#define APPLICATION_SECONDARY_START_ADDRESS                                    \
    ((uint32_t)(uintptr_t) & __app_secondary_start__)
#define APPLICATION_SECONDARY_SIZE                                             \
    ((uint32_t)(uintptr_t) & __app_secondary_size__)

#define WRITER_CONFIG { \
        .slot_addr  = APPLICATION_SECONDARY_START_ADDRESS, \
        .slot_size  = APPLICATION_SECONDARY_SIZE, \
        .erase_size = 0u, \
        .prog_size  = 0u, \
    };

static IapWriterStorageOpsType IapWriterStorageOperations = {0};
static IapWriterContextType IapWriterContext = {0};

static IapIngestAdapterContextType IapIngestAdapterContext;
static const UpdateIngestVTableType *UpdateIngestVTable;

IapErrorType Iap_Init(void)
{
    IapErrorType res = IAP_E_NOT_OK;
    const IapWriterConfigType IapWriterConfig = WRITER_CONFIG;

    FlashStatusType FlashRes;
    IapWriterStatusType IapWriterRes;
    UpdateIngestStatusType UpdateIngestRes;
    UpdateIngestStatusType UpdateIngestStatusRes;

    FlashRes = Flash_Init();

    if (FLASH_E_OK == FlashRes)
    {
        res = IAP_E_OK;
    }

    if (IAP_E_OK == res)
    {
        IapFlashAdapter_InitOps(&IapWriterStorageOperations);
    }

    if (IAP_E_OK == res)
    {
        IapWriterRes = IapWriter_Init(
            &IapWriterContext, 
            &IapWriterConfig, 
            &IapWriterStorageOperations);
        
        if (IAP_WRITER_E_OK != IapWriterRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    if (IAP_E_OK == res)
    {
        UpdateIngestStatusRes = IapIngestAdapter_Init(
            &IapIngestAdapterContext, 
            &IapWriterContext);

        if (UPDATE_INGEST_E_OK != UpdateIngestStatusRes)
        {
            res = IAP_E_NOT_OK;
        }
    }

    if (IAP_E_OK == res)
    {
        UpdateIngestVTable = IapIngestAdapter_GetVTable();
        
        UpdateIngestRes = UpdateIngestRegistry_Register(
            UpdateIngestVTable, 
            (void *)&IapIngestAdapterContext);
        
        if (UPDATE_INGEST_E_OK != UpdateIngestRes)
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

    return res;
}
