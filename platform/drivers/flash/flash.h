#pragma once

#include <stdint.h>

int32_t Flash_Read(uint32_t addr, void *dst, uint32_t len);

int32_t Flash_Write(uint32_t addr, const void *src, uint32_t len);

int32_t Flash_EraseSector(uint32_t addr);
