#pragma once

#include <stdint.h>

#include "bootutil/bootutil.h"

void test_boot_platform_reset(void);
int test_boot_platform_was_called(void);
uint32_t test_boot_platform_image_off(void);
uint8_t test_boot_platform_flash_dev_id(void);
