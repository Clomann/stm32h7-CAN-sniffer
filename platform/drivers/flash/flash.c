#include "flash.h"

#include <string.h>

int32_t Flash_Read(uint32_t addr, void *dst, uint32_t len)
{
    if (dst == NULL)
    {
        return -1;
    }

    memcpy(dst, (const void *)(uintptr_t)addr, len);
    return 0;
}

int32_t Flash_Write(uint32_t addr, const void *src, uint32_t len)
{
    (void)addr;
    (void)src;
    (void)len;
    return -1;
}

int32_t Flash_EraseSector(uint32_t addr)
{
    (void)addr;
    return -1;
}
