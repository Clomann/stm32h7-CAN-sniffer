#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bootloader.h"

#ifndef TEST_FLASH_SIZE
#define TEST_FLASH_SIZE (4u * 1024u * 1024u)
#endif

#ifndef TEST_FLASH_BASE
#define TEST_FLASH_BASE 0u
#endif

#ifndef TEST_FLASH_SECTOR_SIZE
#define TEST_FLASH_SECTOR_SIZE (128u * 1024u)
#endif

static uint8_t test_flash[TEST_FLASH_SIZE];
static int test_flash_initialized;

static void test_flash_init_once(void)
{
    if (!test_flash_initialized)
    {
        memset(test_flash, 0xff, sizeof(test_flash));
        test_flash_initialized = 1;
    }
}

static int test_flash_bounds_ok(uint32_t addr, uint32_t len, uint32_t *off_out)
{
    if (addr < TEST_FLASH_BASE)
    {
        return 0;
    }

    const uint32_t off = addr - TEST_FLASH_BASE;
    if ((uint64_t)off + len > sizeof(test_flash))
    {
        return 0;
    }

    if (off_out != NULL)
    {
        *off_out = off;
    }
    return 1;
}

int boot_internal_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    uint32_t off = 0;

    if (dst == NULL)
    {
        return -1;
    }

    test_flash_init_once();
    if (!test_flash_bounds_ok(addr, len, &off))
    {
        return -1;
    }

    memcpy(dst, &test_flash[off], len);
    return 0;
}

void boot_internal_flash_write(uint32_t addr, const void *src, uint32_t len)
{
    uint32_t off = 0;

    if (src == NULL)
    {
        return;
    }

    test_flash_init_once();
    if (!test_flash_bounds_ok(addr, len, &off))
    {
        assert(0);
        return;
    }

    memcpy(&test_flash[off], src, len);
}

void boot_internal_flash_erase_sector(const uint32_t addr)
{
    uint32_t off = 0;

    test_flash_init_once();
    if (!test_flash_bounds_ok(addr, TEST_FLASH_SECTOR_SIZE, &off))
    {
        assert(0);
        return;
    }

    memset(&test_flash[off], 0xff, TEST_FLASH_SECTOR_SIZE);
}
