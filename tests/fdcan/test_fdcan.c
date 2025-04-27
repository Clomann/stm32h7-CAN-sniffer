#include "unity.h"
#include "fdcan_utils.h"  // <-- your header file to test (adjust to your real one)

void setUp(void)
{
    // This function is called **before each test**.
}

void tearDown(void)
{
    // This function is called **after each test**.
}

// Example function under test:
static int add_numbers(int a, int b)
{
    return a + b;
}

// --- Actual test cases ---

void test_add_numbers_should_add_two_positive_numbers(void)
{
    int result = add_numbers(2, 3);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void bittiming(void)
{
    uint8_t timings[4];
    uint8_t seg1;
    uint8_t seg2;
    uint8_t sjw;
    uint8_t prscl;

    int result = CANFD_CalculateBitTimingRegister(40000000, 250000, 0, &timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, prscl, "Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(35, seg1, "Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, seg2, "Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sjw, "Bit timinng sjw");
}

// --- Main function to run tests ---

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(bittiming);

    return UNITY_END();
}
