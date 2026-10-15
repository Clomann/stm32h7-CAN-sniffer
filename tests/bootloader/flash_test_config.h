#pragma once

#include <stdint.h>

#ifndef TEST_FLASH_SIZE
#define TEST_FLASH_SIZE (4u * 1024u * 1024u)
#endif

#ifndef TEST_FLASH_BASE
#define TEST_FLASH_BASE 0u
#endif

#ifndef TEST_FLASH_SECTOR_SIZE
#define TEST_FLASH_SECTOR_SIZE (128u * 1024u)
#endif

#ifndef TEST_FLASH_PROG_WORDS
#define TEST_FLASH_PROG_WORDS 8u
#endif
