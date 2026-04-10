#include "flash.h"

#include <stdint.h>
#include <string.h>

#include "stm32h745xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_flash.h"
#include "stm32h7xx_hal_flash_ex.h"
#include "stm32h7xx_hal_rcc_ex.h"

#define FLASH_FLASHWORD_SIZE                                                   \
    (FLASH_NB_32BITWORD_IN_FLASHWORD * sizeof(uint32_t))

#define FLASH_ACCESS_TYPE_INV   0
#define FLASH_ACCESS_TYPE_READ  1
#define FLASH_ACCESS_TYPE_WRITE 2
#define FLASH_ACCESS_TYPE_ERASE 3
#define FLASH_ACCESS_TYPE_MAX   4

typedef uint8_t FlashAccessType;

static FLASH_EraseInitTypeDef EraseInitStruct;
static FlashInfoType Info;

FlashStatusType Flash_EraseSector(uint32_t addr);

static FlashStatusType
AddressCheck(uint32_t addr, size_t len, FlashAccessType type)
{
    FlashStatusType res = FLASH_E_OK;

    if (0 == len || len > UINT32_MAX)
    {
        return FLASH_E_PARAM;
    }

    if (addr < Info.base_addr || Info.total_size < len)
    {
        return FLASH_E_RANGE;
    }

    if (addr > UINT32_MAX - len || Info.end_addr < addr + len)
    {
        return FLASH_E_RANGE;
    }

    switch (type)
    {
    case FLASH_ACCESS_TYPE_READ:
        break;
    case FLASH_ACCESS_TYPE_WRITE:
        if (addr % Info.write_alignment != 0 || Info.prog_size_min == 0)
        {
            res = FLASH_E_PARAM;
        }
        else if (addr % Info.prog_size_min != 0
                 || len % Info.prog_size_min != 0)
        {
            res = FLASH_E_ALIGN;
        }
        break;
    case FLASH_ACCESS_TYPE_ERASE:
        if (addr % Info.erase_size_min != 0 || len < Info.erase_size_min
            || len % Info.sector_size != 0)
        {
            res = FLASH_E_ALIGN;
        }
        break;
    // errors
    case FLASH_ACCESS_TYPE_INV:
    case FLASH_ACCESS_TYPE_MAX:
    default:
        res = FLASH_E_PARAM;
        break;
    }

    return res;
}

FlashStatusType Flash_Init(void)
{
    Info.base_addr        = FLASH_BASE;
    Info.total_size       = FLASH_END - FLASH_BASE + 1U;
    Info.end_addr         = FLASH_END + 1U;
    Info.sector_size      = FLASH_SECTOR_SIZE;
    Info.write_alignment  = FLASH_FLASHWORD_SIZE;
    Info.prog_size_min    = (FLASH_NB_32BITWORD_IN_FLASHWORD * 32U) / 8U;
    Info.erase_size_min   = Info.sector_size;
    Info.source_alignment = 4U;

    HAL_FLASH_Unlock();

    return FLASH_E_OK;
}

FlashStatusType Flash_GetInfo(FlashInfoType *info)
{
    if (info == NULL)
    {
        return FLASH_E_PARAM;
    }

    if (Info.total_size == 0U)
    {
        (void)Flash_Init();
    }

    *info = Info;
    return FLASH_E_OK;
}

FlashStatusType Flash_Read(uint32_t addr, void *dst, size_t len)
{
    FlashStatusType res = FLASH_E_OK;

    res = AddressCheck(addr, len, FLASH_ACCESS_TYPE_READ);

    if (FLASH_E_OK != res)
    {
        return res;
    }

    if (NULL == dst)
    {
        return FLASH_E_PARAM;
    }

    memcpy(dst, (const void *)addr, len);

    return FLASH_E_OK;
}

FlashStatusType Flash_Erase(uint32_t addr, size_t len)
{
    if (Info.total_size == 0U)
    {
        (void)Flash_Init();
    }

    FlashStatusType res = AddressCheck(addr, len, FLASH_ACCESS_TYPE_ERASE);
    if (FLASH_E_OK != res)
    {
        return res;
    }

    const size_t sector_size = Info.sector_size;
    for (size_t off = 0; off < len; off += sector_size)
    {
        res = Flash_EraseSector(addr + (uint32_t)off);
        if (FLASH_E_OK != res)
        {
            return res;
        }
    }

    return FLASH_E_OK;
}

FlashStatusType Flash_Write(uint32_t addr, const void *src, size_t len)
{
    HAL_StatusTypeDef HalRes = 0;
    uint32_t FlashWordOffset = 0;
    uint32_t DataAddress     = 0;
    uintptr_t Source         = (uintptr_t)src;
    FlashStatusType res      = FLASH_E_OK;

    res = AddressCheck(addr, len, FLASH_ACCESS_TYPE_WRITE);

    if (FLASH_E_OK != res)
    {
        return res;
    }

    if (NULL == src || len > UINT32_MAX)
    {
        return FLASH_E_PARAM;
    }

    if (Source % Info.source_alignment != 0U || len % Info.prog_size_min != 0
        || len % FLASH_FLASHWORD_SIZE != 0)
    {
        return FLASH_E_ALIGN;
    }

    HalRes = HAL_ERROR;

    while (FlashWordOffset < len)
    {
        DataAddress = (uint32_t)Source + FlashWordOffset;

        HalRes = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_FLASHWORD,
            addr + FlashWordOffset,
            DataAddress
        );

        if (HalRes != HAL_OK)
        {
            break;
        }

        FlashWordOffset += FLASH_FLASHWORD_SIZE;
    }

    if (HAL_OK == HalRes)
    {
        return FLASH_E_OK;
    }
    else
    {
        return FLASH_E_HW;
    }
}

FlashStatusType Flash_EraseSector(uint32_t addr)
{
    FlashStatusType res      = FLASH_E_OK;
    HAL_StatusTypeDef HalRes = HAL_OK;
    uint32_t SectorError     = 0U;

    res = AddressCheck(addr, Info.sector_size, FLASH_ACCESS_TYPE_ERASE);

    if (FLASH_E_OK != res)
    {
        return res;
    }

    if (addr > UINT32_MAX - Info.sector_size
        || addr + Info.sector_size > Info.end_addr)
    {
        return FLASH_E_RANGE;
    }

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;

    if (addr >= FLASH_BANK1_BASE && addr < FLASH_BANK2_BASE)
    {
        EraseInitStruct.Banks = FLASH_BANK_1;
    }
    else if (addr >= FLASH_BANK2_BASE && addr <= FLASH_END)
    {
        EraseInitStruct.Banks = FLASH_BANK_2;
    }
    else
    {
        EraseInitStruct.Banks = 0;
        res                   = FLASH_E_PARAM;
    }

    if (FLASH_E_OK == res)
    {
        if (FLASH_BANK_1 == EraseInitStruct.Banks)
        {
            EraseInitStruct.Sector =
                (addr - FLASH_BANK1_BASE) / Info.sector_size;
        }
        else if (FLASH_BANK_2 == EraseInitStruct.Banks)
        {
            EraseInitStruct.Sector =
                (addr - FLASH_BANK2_BASE) / Info.sector_size;
        }

        EraseInitStruct.NbSectors    = 1U;
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_4;

        HalRes = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

        if (HAL_OK != HalRes)
        {
            res = FLASH_E_HW;
        }
    }

    return res;
}

FlashStatusType
Flash_GetSectorByAddr(uint32_t addr, FlashSectorInfoType *sector)
{
    if (sector == NULL)
    {
        return FLASH_E_PARAM;
    }

    if (addr < FLASH_BASE || addr >= (FLASH_BASE + FLASH_SIZE))
    {
        return FLASH_E_RANGE;
    }

    uint32_t dev_off     = addr - FLASH_BASE;
    uint32_t sector_size = Info.sector_size;

    sector->off  = (dev_off / sector_size) * sector_size;
    sector->size = sector_size;

    return FLASH_E_OK;
}
