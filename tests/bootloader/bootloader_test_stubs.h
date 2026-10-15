#pragma once

#include <stddef.h>
#include <stdint.h>

#include "TestFlashMemory.h"

extern TestFlashMemoryType g_test_flash_memory;

void test_flash_reset(void);
int test_flash_load_area_from_file(uint8_t area_id, const char *path);
int test_flash_load_area_from_buffer(
    uint8_t area_id,
    const uint8_t *data,
    size_t len
);
