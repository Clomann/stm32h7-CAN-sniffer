#include <stddef.h>
#include <stdint.h>

#include "flash.h"

extern uintptr_t g_flash_write_src_full;
extern uint32_t g_flash_write_src_low;
extern int g_flash_write_active;

FlashStatusType Flash_Write_impl(uint32_t addr, const void *src, size_t len);

FlashStatusType Flash_Write(uint32_t addr, const void *src, size_t len)
{
    g_flash_write_src_full = (uintptr_t)src;
    g_flash_write_src_low  = (uint32_t)g_flash_write_src_full;
    g_flash_write_active   = 1;
    FlashStatusType res    = Flash_Write_impl(addr, src, len);
    g_flash_write_active   = 0;
    return res;
}
