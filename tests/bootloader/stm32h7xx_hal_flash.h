#pragma once

#include <stdint.h>

#include "stm32h7xx_hal_def.h"

#define FLASH_TYPEPROGRAM_FLASHWORD 0x01u
#define FLASH_TYPEERASE_SECTORS     0x02u

#define FLASH_BANK_1 1u
#define FLASH_BANK_2 2u

#define FLASH_VOLTAGE_RANGE_4 4u

typedef struct
{
    uint32_t TypeErase;
    uint32_t Banks;
    uint32_t Sector;
    uint32_t NbSectors;
    uint32_t VoltageRange;
} FLASH_EraseInitTypeDef;

HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef
HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t DataAddress);
HAL_StatusTypeDef
HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError);
