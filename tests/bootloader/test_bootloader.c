#include "unity.h"

#include "logging_stub.h"
#include "bootloader_test_stubs.h"
#include "boot_platform_stub.h"
#include "bootloader.h"
#include "bootutil.h"
#include "bootutil/image.h"
#include "image_test_markers.h"
#include "sysflash/sysflash.h"

static FILE *s_log_file;

#ifndef IMAGE_TEST_LOG_PATH
#define IMAGE_TEST_LOG_PATH "bootloader_test.log"
#endif

static void load_signed_image_or_fail(void)
{
#ifndef IMAGE_TEST_SIGNED_BIN_PATH
#define IMAGE_TEST_SIGNED_BIN_PATH "_bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed.bin"
#endif

    static const char *k_paths[] = {
        IMAGE_TEST_SIGNED_BIN_PATH,
        "_bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed.bin",
        "../_bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed.bin",
        "../../_bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed.bin",
    };

    int rc = -1;
    for (size_t i = 0; i < (sizeof(k_paths) / sizeof(k_paths[0])); ++i)
    {
        rc = test_flash_load_area_from_file(
            FLASH_AREA_IMAGE_PRIMARY(0),
            k_paths[i]
        );
        if (rc == 0)
        {
            break;
        }
    }

    TEST_ASSERT_EQUAL_MESSAGE(
        0,
        rc,
        "Signed image not found. Ensure bootloader test signing step completed."
    );
}

void setUp(void)
{
    test_flash_reset();
    test_boot_platform_reset();
    s_log_file = fopen(IMAGE_TEST_LOG_PATH, "w");
    test_log_set_file(s_log_file);
    load_signed_image_or_fail();
}

void tearDown(void)
{
    if (s_log_file != NULL)
    {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    test_log_set_file(NULL);
}

static void test_bootloader_run_calls_boot_go_and_returns_result(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_bootloader_run_calls_boot_go_and_returns_result);

    return UNITY_END();
}

static void test_bootloader_run_calls_boot_go_and_returns_result(void)
{
    int rc = bootloader_run();

    TEST_ASSERT_TRUE(FIH_EQ(rc, FIH_SUCCESS));
    TEST_ASSERT_TRUE(test_boot_platform_was_called());

    const uint32_t image_off = test_boot_platform_image_off();
    struct image_header hdr;
    int read_rc = boot_internal_flash_read(image_off, &hdr, sizeof(hdr));
    TEST_ASSERT_EQUAL(0, read_rc);

    /* Support both layouts:
     * 1) vector table at image_off + ih_hdr_size (default imgtool flow)
     * 2) vector table at image_off + 2*ih_hdr_size (--pad-header input)
     */
    uint32_t reset_word = 0u;
    const uint32_t expected_reset = IMAGE_TEST_RESET_HANDLER_ADDR & ~1u;

    read_rc = boot_internal_flash_read(
        image_off + hdr.ih_hdr_size + 4u,
        &reset_word,
        sizeof(reset_word)
    );
    TEST_ASSERT_EQUAL(0, read_rc);

    // IMAGE_TEST_RESET_HANDLER_ADDR is provided by cmake
    // and generated every build to tests/build/image_test_markers.h
    if ((reset_word & ~1u) != expected_reset)
    {
        read_rc = boot_internal_flash_read(
            image_off + (2u * hdr.ih_hdr_size) + 4u,
            &reset_word,
            sizeof(reset_word)
        );
        TEST_ASSERT_EQUAL(0, read_rc);
    }

    TEST_ASSERT_EQUAL_HEX32(expected_reset, reset_word & ~1u);
}
