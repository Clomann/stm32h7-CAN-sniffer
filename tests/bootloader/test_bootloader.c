#include "unity.h"

#include "logging_stub.h"
#include "bootloader_test_stubs.h"
#include "boot_platform_stub.h"
#include "bootloader.h"
#include "bootutil.h"
#include "bootutil/image.h"
#include "image_test_markers.h"
#include "sysflash/sysflash.h"
#include "flash_map_backend/flash_map_backend.h"

#include <stdio.h>
#include <stdlib.h>

static FILE *s_log_file;
extern void *test_flash_memcpy(void *dst, const void *src, size_t len);

#ifndef IMAGE_TEST_LOG_PATH
#define IMAGE_TEST_LOG_PATH "bootloader_test.log"
#endif

#ifndef IMAGE_TEST_SIGNED_BIN_PATH
#define IMAGE_TEST_SIGNED_BIN_PATH "tests/build/SPI_FullDuplex_ComDMA_CM7_app-signed.bin"
#endif

#ifndef IMAGE_TEST_SIGNED_ENCRYPTED_BIN_PATH
#define IMAGE_TEST_SIGNED_ENCRYPTED_BIN_PATH "tests/build/SPI_FullDuplex_ComDMA_CM7_app-signed-encrypted.bin"
#endif

static void load_image_or_fail(uint8_t area_id, const char *path, const char *what)
{
    FILE *f = fopen(path, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, what);

    const int seek_end_rc = fseek(f, 0L, SEEK_END);
    TEST_ASSERT_EQUAL_MESSAGE(0, seek_end_rc, what);

    const long file_size = ftell(f);
    TEST_ASSERT_TRUE_MESSAGE(file_size > 0L, what);

    const int seek_set_rc = fseek(f, 0L, SEEK_SET);
    TEST_ASSERT_EQUAL_MESSAGE(0, seek_set_rc, what);

    uint8_t *buf = (uint8_t *)malloc((size_t)file_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, what);

    const size_t read_n = fread(buf, 1, (size_t)file_size, f);
    TEST_ASSERT_EQUAL_MESSAGE((size_t)file_size, read_n, what);
    fclose(f);

    const struct flash_area *fa = NULL;
    const int open_rc = flash_area_open(area_id, &fa);
    TEST_ASSERT_EQUAL_MESSAGE(0, open_rc, what);
    TEST_ASSERT_NOT_NULL_MESSAGE(fa, what);
    TEST_ASSERT_TRUE_MESSAGE((uint32_t)file_size <= fa->fa_size, what);

    (void)test_flash_memcpy((void *)(uintptr_t)fa->fa_off, buf, (size_t)file_size);

    uint8_t *verify = (uint8_t *)malloc((size_t)file_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(verify, what);
    const int read_rc = boot_internal_flash_read(
        fa->fa_off,
        verify,
        (uint32_t)file_size
    );
    TEST_ASSERT_EQUAL_MESSAGE(0, read_rc, what);
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        buf,
        verify,
        (size_t)file_size,
        "Loaded image differs from test artifact bytes."
    );
    free(verify);
    flash_area_close(fa);

    const int rc = 0;
    free(buf);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, what);
}

static void load_signed_primary_or_fail(void)
{
    load_image_or_fail(
        FLASH_AREA_IMAGE_PRIMARY(0),
        IMAGE_TEST_SIGNED_BIN_PATH,
        "Signed primary image not found. Ensure bootloader test signing step completed."
    );
}

static void load_signed_secondary_or_fail(void)
{
    load_image_or_fail(
        FLASH_AREA_IMAGE_SECONDARY(0),
        IMAGE_TEST_SIGNED_BIN_PATH,
        "Signed secondary image not found. Ensure bootloader test signing step completed."
    );
}

#if defined(MCUBOOT_ENABLE_ENCRYPTION)
static void load_encrypted_secondary_or_fail(void)
{
    load_image_or_fail(
        FLASH_AREA_IMAGE_SECONDARY(0),
        IMAGE_TEST_SIGNED_ENCRYPTED_BIN_PATH,
        "Signed+encrypted secondary image not found. Ensure bootloader test signing step completed."
    );
}
#endif

static void assert_boot_success_and_reset_vector(void)
{
    const int rc = bootloader_main();

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

void setUp(void)
{
    test_flash_reset();
    test_boot_platform_reset();
    s_log_file = fopen(IMAGE_TEST_LOG_PATH, "w");
    test_log_set_file(s_log_file);
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

static void test_bootloader_run_with_signed_secondary_image(void);
#if defined(MCUBOOT_ENABLE_ENCRYPTION)
static void test_bootloader_run_with_encrypted_secondary_image(void);
#endif

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_bootloader_run_with_signed_secondary_image);
#if defined(MCUBOOT_ENABLE_ENCRYPTION)
    RUN_TEST(test_bootloader_run_with_encrypted_secondary_image);
#endif

    return UNITY_END();
}

static void test_bootloader_run_with_signed_secondary_image(void)
{
    load_signed_primary_or_fail();
    load_signed_secondary_or_fail();
    assert_boot_success_and_reset_vector();
}

#if defined(MCUBOOT_ENABLE_ENCRYPTION)
static void test_bootloader_run_with_encrypted_secondary_image(void)
{
    load_signed_primary_or_fail();
    load_encrypted_secondary_or_fail();
    assert_boot_success_and_reset_vector();
}
#endif
