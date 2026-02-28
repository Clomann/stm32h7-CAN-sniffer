#include "bootloader.h"
#include "flash.h"

#include <assert.h>

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

WEAK int boot_internal_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    return Flash_Read(addr, dst, len);
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
    if (Flash_EraseSector(addr) != 0)
    {
        assert(0);
    }
}
