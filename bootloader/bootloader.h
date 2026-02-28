#pragma once

#include <stdint.h>

int bootloader_run(void);

int bootloader_main(void);

struct boot_rsp;
void boot_platform_do_boot(const struct boot_rsp *rsp);

int boot_internal_flash_read(uint32_t addr, void *dst, uint32_t len);

void boot_internal_flash_write(uint32_t addr, const void *src, uint32_t len);

void boot_internal_flash_erase_sector(const uint32_t addr);
