#include "bootloader.h"
#include "flash.h"

#include <assert.h>

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

BtlErrorType boot_internal_flash_init(void)
{
    FlashStatusType FlashRes;
    BtlErrorType rv;

    FlashRes = Flash_Init();
    
    if (FLASH_E_OK == FlashRes)
    {
        rv = BTL_E_OK;
    }
    else 
    {
        rv = BTL_E_NOT_OK;
    }

    return rv;
}


WEAK BtlErrorType boot_internal_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    BtlErrorType rv;

    if (FLASH_E_OK == Flash_Read(addr, dst, len))
    {
        rv = BTL_E_OK;
    }
    else 
    {
        rv = BTL_E_NOT_OK;
    }
    
    return rv;
}

WEAK void
boot_internal_flash_write(uint32_t addr, const void *src, uint32_t len)
{
    if (Flash_Write(addr, src, len) != 0)
    {
        assert(0);
    }
}

WEAK void boot_internal_flash_erase_sector(const uint32_t addr)
{
    FlashInfoType Info;

    Flash_GetInfo(&Info);

    if (Flash_Erase(addr, Info.sector_size) != 0)
    {
        assert(0);
    }
}
