#pragma once

#include <stddef.h>
#include <string.h>

#ifdef memcpy
#undef memcpy
#endif
#define memcpy test_flash_memcpy

void *test_flash_memcpy(void *dst, const void *src, size_t len);
