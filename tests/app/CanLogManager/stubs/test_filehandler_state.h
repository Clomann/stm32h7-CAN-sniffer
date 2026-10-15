#pragma once

#include <stdint.h>
#include <stdbool.h>

void test_filehandler_set_meta_content(const char *data, uint32_t len);
void test_filehandler_clear_meta_content(void);
const char *test_filehandler_get_meta_content(uint32_t *len);
bool test_filehandler_meta_exists(void);
uint32_t test_filehandler_meta_size(void);
