#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bootloader.h"
#include "bootloader_test_stubs.h"
#include "flash.h"
#include "flash_test_config.h"
#include "flash_map_backend/flash_map_backend.h"
#include "TestFlashMemory.h"
uint8_t test_flash[TEST_FLASH_SIZE];
int test_flash_initialized;
TestFlashMemoryType g_test_flash_memory;

uintptr_t g_flash_write_src_full;
uint32_t g_flash_write_src_low;
int g_flash_write_active;

static void test_flash_init_once(void)
{
    if (!test_flash_initialized)
    {
        TestFlashMemory_Init(
            &g_test_flash_memory,
            test_flash,
            sizeof(test_flash),
            TEST_FLASH_BASE
        );
        TestFlashMemory_Reset(&g_test_flash_memory, 0xFF);
        test_flash_initialized = 1;
    }
}

void *test_flash_memcpy(void *dst, const void *src, size_t len)
{
    if (len == 0)
    {
        return dst;
    }
    test_flash_init_once();
    return TestFlashMemory_Memcpy(&g_test_flash_memory, dst, src, len);
}

void test_flash_reset(void)
{
    TestFlashMemory_Init(
        &g_test_flash_memory,
        test_flash,
        sizeof(test_flash),
        TEST_FLASH_BASE
    );
    TestFlashMemory_Reset(&g_test_flash_memory, 0xFF);
    test_flash_initialized = 1;
    (void)Flash_Init();
}

static int
test_flash_write_padded(uint32_t addr, const uint8_t *data, size_t len)
{
    if (!test_flash_initialized)
    {
        test_flash_reset();
    }

    FlashInfoType info;
    size_t prog_size = 1u;
    if (Flash_GetInfo(&info) == FLASH_E_OK && info.prog_size_min != 0u)
    {
        prog_size = info.prog_size_min;
    }

    size_t remaining      = len;
    const uint8_t *cursor = data;
    const size_t chunk    = 512u;
    uint8_t *buf          = malloc(chunk + prog_size);
    if (buf == NULL)
    {
        return -1;
    }

    while (remaining > 0u)
    {
        const size_t n = remaining > chunk ? chunk : remaining;
        memcpy(buf, cursor, n);

        size_t write_len = n;
        if (prog_size > 1u && (write_len % prog_size) != 0u)
        {
            const size_t padded =
                ((write_len + prog_size - 1u) / prog_size) * prog_size;
            memset(buf + write_len, 0xff, padded - write_len);
            write_len = padded;
        }

        g_flash_write_src_full = (uintptr_t)buf;
        g_flash_write_src_low  = (uint32_t)g_flash_write_src_full;
        g_flash_write_active   = 1;
        boot_internal_flash_write(addr, buf, (uint32_t)write_len);
        g_flash_write_active = 0;
        addr += (uint32_t)write_len;
        cursor += n;
        remaining -= n;
    }

    free(buf);
    return 0;
}

int test_flash_load_area_from_file(uint8_t area_id, const char *path)
{
    if (path == NULL)
    {
        return -1;
    }

    const struct flash_area *fa = NULL;
    if (flash_area_open(area_id, &fa) != 0 || fa == NULL)
    {
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        flash_area_close(fa);
        return -1;
    }

    uint8_t buf[512];
    size_t n      = 0;
    uint32_t addr = fa->fa_off;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0u)
    {
        if (test_flash_write_padded(addr, buf, n) != 0)
        {
            fclose(f);
            flash_area_close(fa);
            return -1;
        }
        addr += (uint32_t)n;
    }

    fclose(f);
    flash_area_close(fa);
    return 0;
}

int test_flash_load_area_from_buffer(
    uint8_t area_id,
    const uint8_t *data,
    size_t len
)
{
    if (data == NULL)
    {
        return -1;
    }

    const struct flash_area *fa = NULL;
    if (flash_area_open(area_id, &fa) != 0 || fa == NULL)
    {
        return -1;
    }

    const int rc = test_flash_write_padded(fa->fa_off, data, len);
    flash_area_close(fa);
    return rc;
}
