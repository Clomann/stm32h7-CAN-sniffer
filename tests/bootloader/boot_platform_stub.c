#include "boot_platform_stub.h"

static int s_called;
static uint32_t s_image_off;
static uint8_t s_flash_dev_id;

void test_boot_platform_reset(void)
{
    s_called       = 0;
    s_image_off    = 0u;
    s_flash_dev_id = 0u;
}

int test_boot_platform_was_called(void)
{
    return s_called;
}

uint32_t test_boot_platform_image_off(void)
{
    return s_image_off;
}

uint8_t test_boot_platform_flash_dev_id(void)
{
    return s_flash_dev_id;
}

void boot_platform_do_boot(const struct boot_rsp *rsp)
{
    s_called = 1;
    if (rsp == NULL)
    {
        return;
    }

    s_image_off    = rsp->br_image_off;
    s_flash_dev_id = rsp->br_flash_dev_id;
}
