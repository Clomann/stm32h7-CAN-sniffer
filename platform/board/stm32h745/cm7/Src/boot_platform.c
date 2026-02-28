#include <stdint.h>

#include "bootloader.h"
#include "bootutil/bootutil.h"
#include "stm32h7xx.h"

typedef void (*app_entry_t)(void);

void boot_platform_do_boot(const struct boot_rsp *rsp)
{
    if (rsp == NULL)
    {
        return;
    }

    uint32_t addr  = FLASH_BASE + rsp->br_image_off;
    uint32_t msp   = *(uint32_t *)addr;
    uint32_t reset = *(uint32_t *)(addr + 4U);

    __disable_irq();
    SCB->VTOR = addr;
    __set_MSP(msp);
    __DSB();
    __ISB();

    ((app_entry_t)reset)();

    while (1)
    {
    }
}
