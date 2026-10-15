#include "bootutil.h"
#include "bootloader.h"
#include "mbedtls/memory_buffer_alloc.h"

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

static unsigned char mbedtls_heap[128U * 1024U];

static BtlErrorType bootloader_init(void);

WEAK void boot_platform_do_boot(const struct boot_rsp *rsp)
{
    (void)rsp;
    (void)0;
}

void *boot_static_calloc(size_t num, size_t size)
{
    (void)num;
    (void)size;
    return NULL;
}

void boot_static_free(void *ptr)
{
    (void)ptr;
}

static BtlErrorType bootloader_init(void)
{
    BtlErrorType rv;

    rv = boot_internal_flash_init();

    return rv;
}

BtlErrorType bootloader_run(void)
{
    struct boot_rsp rsp;
    BtlErrorType rv;
    fih_ret rv_boot;

    rv = bootloader_init();

    if (BTL_E_OK == rv)
    {
        boot_activate_pending_if_image_present();
        rv_boot = boot_go(&rsp);

        if (FIH_EQ(rv_boot, FIH_SUCCESS))
        {
            rv = BTL_E_OK;
        }
        else
        {
            rv = BTL_E_NOT_OK;
        }
    }

    if (BTL_E_OK == rv)
    {
        boot_platform_do_boot(&rsp);
    }

    return rv;
}

BtlErrorType bootloader_main(void)
{
    mbedtls_memory_buffer_alloc_init(mbedtls_heap, sizeof(mbedtls_heap));

    return bootloader_run();
}

void example_assert_handler(const char *file, int line)
{
    (void)file;
    (void)line;
}
