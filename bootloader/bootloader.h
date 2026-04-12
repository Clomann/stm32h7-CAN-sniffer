#pragma once

#include <stdint.h>
#include <stdio.h>

#define BTL_E_OK     0U
#define BTL_E_NOT_OK 1U

typedef uint8_t BtlErrorType;

struct boot_rsp;

BtlErrorType bootloader_run(void);

BtlErrorType bootloader_main(void);

void boot_platform_do_boot(const struct boot_rsp *rsp);

BtlErrorType boot_internal_flash_init(void);

BtlErrorType boot_internal_flash_read(uint32_t addr, void *dst, uint32_t len);

void boot_internal_flash_write(uint32_t addr, const void *src, uint32_t len);

void boot_internal_flash_erase_sector(const uint32_t addr);

void *boot_static_calloc(size_t num, size_t size);

void boot_static_free(void *ptr);
