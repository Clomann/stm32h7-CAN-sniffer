#include "FwUpdateHandoff.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "flash.h"

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

#define FW_UPDATE_HANDOFF_MAGIC         (0x46574844u) /* "FWHD" */
#define FW_UPDATE_HANDOFF_VERSION       (1u)
#define FW_UPDATE_HANDOFF_REQUEST_APPLY (0x4150504Cu) /* "APPL" */
#define FW_UPDATE_HANDOFF_CRC_SALT      (0xA5A5A5A5u)

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t request;
    uint32_t request_inv;
    uint32_t crc;
    uint32_t reserved0;
    uint32_t reserved1;
    uint32_t reserved2;
} FwUpdateHandoffRecordType;

static uint32_t FwUpdateHandoff_Crc(const FwUpdateHandoffRecordType *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    return record->magic ^ record->version ^ record->request
           ^ record->request_inv ^ FW_UPDATE_HANDOFF_CRC_SALT;
}

static FwUpdateHandoffStatusType
FwUpdateHandoff_GetMarkerLocation(uint32_t *address, uint32_t *sector_size)
{
    FlashInfoType info = {0};
    uint32_t marker_address;
    FlashStatusType flash_status;

    if (address == NULL || sector_size == NULL)
    {
        return FW_UPDATE_HANDOFF_E_PARAM;
    }

    flash_status = Flash_GetInfo(&info);
    if (flash_status != FLASH_E_OK || info.sector_size == 0u)
    {
        return FW_UPDATE_HANDOFF_E_FLASH;
    }

    if (FW_UPDATE_HANDOFF_E_OK
        != FwUpdateHandoff_GetMarkerAddress_Hook(&marker_address))
    {
        return FW_UPDATE_HANDOFF_E_STATE;
    }

    if (marker_address < info.base_addr
        || marker_address > (UINT32_MAX - info.sector_size)
        || (marker_address + info.sector_size) > info.end_addr
        || (marker_address % info.sector_size) != 0u)
    {
        return FW_UPDATE_HANDOFF_E_STATE;
    }

    *address     = marker_address;
    *sector_size = info.sector_size;
    return FW_UPDATE_HANDOFF_E_OK;
}

static uint8_t
FwUpdateHandoff_RecordIsValid(const FwUpdateHandoffRecordType *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    if (record->magic != FW_UPDATE_HANDOFF_MAGIC
        || record->version != FW_UPDATE_HANDOFF_VERSION
        || record->request != FW_UPDATE_HANDOFF_REQUEST_APPLY
        || record->request_inv != ~FW_UPDATE_HANDOFF_REQUEST_APPLY
        || record->crc != FwUpdateHandoff_Crc(record))
    {
        return 0u;
    }

    return 1u;
}

FwUpdateHandoffStatusType FwUpdateHandoff_RequestApply(void)
{
    FwUpdateHandoffRecordType record = {
        .magic       = FW_UPDATE_HANDOFF_MAGIC,
        .version     = FW_UPDATE_HANDOFF_VERSION,
        .request     = FW_UPDATE_HANDOFF_REQUEST_APPLY,
        .request_inv = ~FW_UPDATE_HANDOFF_REQUEST_APPLY,
        .crc         = 0u,
        .reserved0   = 0u,
        .reserved1   = 0u,
        .reserved2   = 0u,
    };
    FwUpdateHandoffRecordType readback = {0};
    FlashStatusType flash_status;
    uint32_t marker_address = 0u;
    uint32_t sector_size    = 0u;
    FwUpdateHandoffStatusType handoff_status;

    handoff_status =
        FwUpdateHandoff_GetMarkerLocation(&marker_address, &sector_size);
    if (handoff_status != FW_UPDATE_HANDOFF_E_OK)
    {
        return handoff_status;
    }

    record.crc = FwUpdateHandoff_Crc(&record);

    flash_status = Flash_Read(marker_address, &readback, sizeof(readback));
    if (flash_status == FLASH_E_OK && FwUpdateHandoff_RecordIsValid(&readback))
    {
        return FW_UPDATE_HANDOFF_E_OK;
    }

    flash_status = Flash_Erase(marker_address, sector_size);
    if (flash_status != FLASH_E_OK)
    {
        return FW_UPDATE_HANDOFF_E_FLASH;
    }

    flash_status = Flash_Write(marker_address, &record, sizeof(record));
    if (flash_status != FLASH_E_OK)
    {
        return FW_UPDATE_HANDOFF_E_FLASH;
    }

    flash_status = Flash_Read(marker_address, &readback, sizeof(readback));
    if (flash_status != FLASH_E_OK
        || memcmp(&record, &readback, sizeof(record)) != 0)
    {
        return FW_UPDATE_HANDOFF_E_FLASH;
    }

    return FW_UPDATE_HANDOFF_E_OK;
}

FwUpdateHandoffStatusType FwUpdateHandoff_ClearApplyRequest(void)
{
    uint32_t marker_address = 0u;
    uint32_t sector_size    = 0u;
    FwUpdateHandoffStatusType handoff_status;

    handoff_status =
        FwUpdateHandoff_GetMarkerLocation(&marker_address, &sector_size);
    if (handoff_status != FW_UPDATE_HANDOFF_E_OK)
    {
        return handoff_status;
    }

    if (Flash_Erase(marker_address, sector_size) != FLASH_E_OK)
    {
        return FW_UPDATE_HANDOFF_E_FLASH;
    }

    return FW_UPDATE_HANDOFF_E_OK;
}

uint8_t FwUpdateHandoff_IsApplyRequested(void)
{
    FwUpdateHandoffRecordType record = {0};
    uint32_t marker_address          = 0u;
    uint32_t sector_size             = 0u;
    FwUpdateHandoffStatusType handoff_status;

    handoff_status =
        FwUpdateHandoff_GetMarkerLocation(&marker_address, &sector_size);
    if (handoff_status != FW_UPDATE_HANDOFF_E_OK)
    {
        return 0u;
    }

    if (Flash_Read(marker_address, &record, sizeof(record)) != FLASH_E_OK)
    {
        return 0u;
    }

    return FwUpdateHandoff_RecordIsValid(&record);
}

WEAK uint8_t FwUpdateHandoff_GetMarkerAddress_Hook(uint32_t *address)
{
    (void)address;

    return FW_UPDATE_HANDOFF_E_STATE;
}
