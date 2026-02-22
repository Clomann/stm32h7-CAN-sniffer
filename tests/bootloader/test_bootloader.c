#include "unity.h"

#include "bootloader.h"
#include "bootutil.h"

void setUp(void)
{
}

void tearDown(void)
{
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

    TEST_ASSERT_NOT_EQUAL(0, rc);
}
