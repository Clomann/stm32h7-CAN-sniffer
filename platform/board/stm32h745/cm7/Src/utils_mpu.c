#include "utils_mpu.h"
#include <assert.h>
#include <stdint.h>

uint32_t mpu_utils_get_size(uint64_t size)
{
    uint32_t val;

    if (size % 32 != 0)
    {
        return 0;
    }

    switch (size)
    {
    case 1ULL << 5:
        val = MPU_REGION_SIZE_32B;
        break;
    case 1ULL << 6:
        val = MPU_REGION_SIZE_64B;
        break;
    case 1ULL << 7:
        val = MPU_REGION_SIZE_128B;
        break;
    case 1ULL << 8:
        val = MPU_REGION_SIZE_256B;
        break;
    case 1ULL << 9:
        val = MPU_REGION_SIZE_512B;
        break;
    case 1ULL << 10:
        val = MPU_REGION_SIZE_1KB;
        break;
    case 1ULL << 11:
        val = MPU_REGION_SIZE_2KB;
        break;
    case 1ULL << 12:
        val = MPU_REGION_SIZE_4KB;
        break;
    case 1ULL << 13:
        val = MPU_REGION_SIZE_8KB;
        break;
    case 1ULL << 14:
        val = MPU_REGION_SIZE_16KB;
        break;
    case 1ULL << 15:
        val = MPU_REGION_SIZE_32KB;
        break;
    case 1ULL << 16:
        val = MPU_REGION_SIZE_64KB;
        break;
    case 1ULL << 17:
        val = MPU_REGION_SIZE_128KB;
        break;
    case 1ULL << 18:
        val = MPU_REGION_SIZE_256KB;
        break;
    case 1ULL << 19:
        val = MPU_REGION_SIZE_512KB;
        break;
    case 1ULL << 20:
        val = MPU_REGION_SIZE_1MB;
        break;
    case 1ULL << 21:
        val = MPU_REGION_SIZE_2MB;
        break;
    case 1ULL << 22:
        val = MPU_REGION_SIZE_4MB;
        break;
    case 1ULL << 23:
        val = MPU_REGION_SIZE_8MB;
        break;
    case 1ULL << 24:
        val = MPU_REGION_SIZE_16MB;
        break;
    case 1ULL << 25:
        val = MPU_REGION_SIZE_32MB;
        break;
    case 1ULL << 26:
        val = MPU_REGION_SIZE_64MB;
        break;
    case 1ULL << 27:
        val = MPU_REGION_SIZE_128MB;
        break;
    case 1ULL << 28:
        val = MPU_REGION_SIZE_256MB;
        break;
    case 1ULL << 29:
        val = MPU_REGION_SIZE_512MB;
        break;
    case 1ULL << 30:
        val = MPU_REGION_SIZE_1GB;
        break;
    case 1ULL << 31:
        val = MPU_REGION_SIZE_2GB;
        break;
    case 1ULL << 32:
        val = MPU_REGION_SIZE_4GB;
        break;
    default:
        val = 0;
        break;
    }

    return val;
}

uint64_t round_up_pow2(uint64_t v)
{
    if (v < 32)
    {
        v = 32;
    }
    v--; /* 00010000 -> 00001111                */
    v |= v >> 1; /* smear highest bit downwards         */
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v; /* back to next power-of-two           */
}
