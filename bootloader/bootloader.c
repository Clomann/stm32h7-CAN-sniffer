#include "bootutil.h"
#include "bootloader.h"

int bootloader_run(void)
{
    struct boot_rsp rsp;
    int rv = boot_go(&rsp);

    if (rv == 0)
    {
        // 'rsp' contains the start address of the image
        // your_platform_do_boot(&rsp);
    }

    return rv;
}

void example_assert_handler(const char *file, int line)
{
    (void)file;
    (void)line;
}
