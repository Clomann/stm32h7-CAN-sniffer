#pragma once

#include <stdint.h>

#include "stm32h7xx_hal.h"

uint64_t round_up_pow2(uint64_t v);
uint32_t mpu_utils_get_size(uint64_t size);
