#include "unity.h"
#include "fdcan_utils.h"

void setUp(void)
{
    // This function is called **before each test**.
}

void tearDown(void)
{
    // This function is called **after each test**.
}

void bittiming(void);

// --- Main function to run tests ---

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(bittiming);
    
    return UNITY_END();
}

void bittiming(void)
{
    uint8_t timings[4];
    uint8_t seg1;
    uint8_t seg2;
    uint8_t sjw;
    uint8_t prscl;
    int result;

    result = CANFD_CalculateBitTimingRegister(40000000, 250000, 8750, 1, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 1: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(139, seg1, "Test 1: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(20, seg2, "Test 1: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sjw, "Test 1: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 250000, 7500, 1, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 2: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(119, seg1, "Test 2: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(40, seg2, "Test 2: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sjw, "Test 2: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 500000, 7500, 1, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 3: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(59, seg1, "Test 3: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(20, seg2, "Test 3: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sjw, "Test 3: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 500000, 7500, 10, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 4: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(59, seg1, "Test 4: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(20, seg2, "Test 4: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, sjw, "Test 4: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 1000000, 7500, 1, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 5: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(29, seg1, "Test 5: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, seg2, "Test 5: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, sjw, "Test 5: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 1000000, 7500, 4, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 6: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(29, seg1, "Test 6: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, seg2, "Test 6: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, sjw, "Test 6: Bit timinng sjw");

    result = CANFD_CalculateBitTimingRegister(40000000, 1000000, 7500, 10, 0, timings);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "");

    prscl = CANFD_GetPrescaler(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, prscl, "Test 7: Bit timing prescaler");

    seg1 = CANFD_GetSeg1(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(29, seg1, "Test 7: Bit timinng seg1");

    seg2 = CANFD_GetSeg2(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, seg2, "Test 7: Bit timinng seg2");

    sjw = CANFD_GetSJW(timings, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(10, sjw, "Test 7: Bit timinng sjw");
}