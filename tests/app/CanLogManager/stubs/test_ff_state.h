#pragma once

#include <stdint.h>
#include <stdbool.h>

void test_ff_reset(void);
bool test_ff_file_exists(const char *path);
uint32_t test_ff_get_file_size(const char *path);
uint32_t test_ff_get_open_count(void);
uint32_t test_ff_get_expand_count(void);
