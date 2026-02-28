#include "bootutil.h"
#include "bootloader.h"

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

WEAK void boot_platform_do_boot(const struct boot_rsp *rsp)
{
    (void)rsp;
}

int bootloader_run(void)
{
    struct boot_rsp rsp;
    int rv = boot_go(&rsp);

    if (rv == 0)
    {
        boot_platform_do_boot(&rsp);
    }

    return rv;
}

int bootloader_main(void)
{
    return bootloader_run();
}

void example_assert_handler(const char *file, int line)
{
    (void)file;
    (void)line;
}
