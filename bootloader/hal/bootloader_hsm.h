#pragma once

#include <stddef.h>
#include <stdint.h>

int32_t Bootloader_Hsm_Init(void);

int32_t Bootloader_Hsm_Verify(
    const uint8_t *data,
    size_t len,
    const uint8_t *sig,
    size_t sig_len
);

void Bootloader_Hsm_Deinit(void);
