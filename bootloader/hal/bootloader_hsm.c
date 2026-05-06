#include "bootloader_hsm.h"

int32_t Bootloader_Hsm_Init(void)
{
    return -1;
}

int32_t Bootloader_Hsm_Verify(
    const uint8_t *data,
    size_t len,
    const uint8_t *sig,
    size_t sig_len
)
{
    (void)data;
    (void)len;
    (void)sig;
    (void)sig_len;
    return -1;
}

void Bootloader_Hsm_Deinit(void)
{
}
