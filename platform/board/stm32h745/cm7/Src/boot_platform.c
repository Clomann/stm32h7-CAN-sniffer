#include <stdint.h>

#include "bootloader.h"
#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "stm32h7xx.h"

typedef void (*app_entry_t)(void);

void boot_platform_do_boot(const struct boot_rsp *rsp)
{
    if (rsp == NULL || rsp->br_hdr == NULL)
    {
        return;
    }

    const uint32_t image_addr  = FLASH_BASE + rsp->br_image_off;
    const uint32_t vector_addr = image_addr + rsp->br_hdr->ih_hdr_size;
    const uint32_t msp         = *(const uint32_t *)vector_addr;
    const uint32_t reset       = *(const uint32_t *)(vector_addr + 4U);

    __disable_irq();
    SCB->VTOR = vector_addr;
    __set_MSP(msp);
    __DSB();
    __ISB();

    ((app_entry_t)reset)();

    while (1)
    {
    }
}
