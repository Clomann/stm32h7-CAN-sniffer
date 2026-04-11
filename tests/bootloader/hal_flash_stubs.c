#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "flash_test_config.h"
#include "stm32h745xx.h"
#include "stm32h7xx_hal_flash.h"

#define TEST_FLASHWORD_SIZE (TEST_FLASH_PROG_WORDS * sizeof(uint32_t))

extern uint8_t test_flash[];
extern int test_flash_initialized;
extern uintptr_t g_flash_write_src_full;
extern uint32_t g_flash_write_src_low;
extern int g_flash_write_active;

static int test_flash_bounds_ok_uintptr(
    uintptr_t addr,
    size_t len,
    uint32_t *off_out
)
{
    if (addr < (uintptr_t)TEST_FLASH_BASE)
    {
        return 0;
    }

    if (addr > UINT32_MAX)
    {
        return 0;
    }

    const uint64_t off =
        (uint64_t)(addr - (uintptr_t)TEST_FLASH_BASE);
    if (off + len > (uint64_t)TEST_FLASH_SIZE)
    {
        return 0;
    }

    if (off_out != NULL)
    {
        *off_out = (uint32_t)off;
    }
    return 1;
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
    return HAL_OK;
}

HAL_StatusTypeDef
HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t DataAddress)
{
    (void)TypeProgram;

    uint32_t off = 0;
    if (!test_flash_bounds_ok_uintptr(Address, TEST_FLASHWORD_SIZE, &off))
    {
        return HAL_ERROR;
    }

    if (!test_flash_initialized)
    {
        memset(test_flash, 0xff, TEST_FLASH_SIZE);
        test_flash_initialized = 1;
    }

    const uint8_t *src = NULL;
    if (g_flash_write_active)
    {
        const uint32_t delta = DataAddress - g_flash_write_src_low;
        src = (const uint8_t *)(g_flash_write_src_full + (uintptr_t)delta);
    }
    else
    {
        src = (const uint8_t *)(uintptr_t)DataAddress;
    }

    for (size_t i = 0; i < TEST_FLASHWORD_SIZE; ++i)
    {
        test_flash[off + i] = src[i];
    }

    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASHEx_Erase(
    FLASH_EraseInitTypeDef *pEraseInit,
    uint32_t *SectorError
)
{
    if (pEraseInit == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t bank_base = 0u;
    if (pEraseInit->Banks == FLASH_BANK_1)
    {
        bank_base = FLASH_BANK1_BASE;
    }
    else if (pEraseInit->Banks == FLASH_BANK_2)
    {
        bank_base = FLASH_BANK2_BASE;
    }
    else
    {
        return HAL_ERROR;
    }

    const uint64_t start_addr =
        (uint64_t)bank_base + ((uint64_t)pEraseInit->Sector
                               * (uint64_t)TEST_FLASH_SECTOR_SIZE);
    const size_t total_len =
        (size_t)pEraseInit->NbSectors * (size_t)TEST_FLASH_SECTOR_SIZE;

    uint32_t off = 0;
    if (!test_flash_bounds_ok_uintptr((uintptr_t)start_addr, total_len, &off))
    {
        if (SectorError != NULL)
        {
            *SectorError = pEraseInit->Sector;
        }
        return HAL_ERROR;
    }

    if (!test_flash_initialized)
    {
        memset(test_flash, 0xff, TEST_FLASH_SIZE);
        test_flash_initialized = 1;
    }

    memset(&test_flash[off], 0xff, total_len);
    if (SectorError != NULL)
    {
        *SectorError = 0u;
    }
    return HAL_OK;
}
