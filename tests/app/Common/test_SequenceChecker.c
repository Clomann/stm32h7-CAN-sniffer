#include "test_SequenceChecker.h"

#include "unity.h"

#include "SequenceChecker.h"

static void RunAllTests(void);

void test_SequenceChecker_setUp(void)
{
}

void test_SequenceChecker_tearDown(void)
{

}

void test_SequenceChecker_HappyPath(void)
{
    ScSequenceType testSc;
    uint32_t MissingCount = 0U;
    ScStatusType res      = SC_E_OK;

    res = ScInit(&testSc, UINT32_MAX, UINT32_MAX);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, 0U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, 1U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScCheckSequence(&testSc, 2U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScReset(&testSc, 0U);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScCheckSequence(&testSc, 1U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, 2U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, 3U);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScDeInit(&testSc);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScReset(&testSc, 0U);
    TEST_ASSERT_EQUAL(SC_E_SEQUENCE, res);
}

void test_SequenceChecker_MissingFrames(void)
{
    ScSequenceType testSc;
    uint32_t MissingCount = 0U;
    ScStatusType res      = SC_E_OK;

    res = ScInit(&testSc, (uint64_t)UINT32_MAX - 3UL, UINT32_MAX);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, (uint64_t)UINT32_MAX - 2UL);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, (uint64_t)UINT32_MAX - 1UL);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScCheckSequence(&testSc, 14UL);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(15U, MissingCount);

    res = ScCheckSequence(&testSc, UINT32_MAX);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(UINT32_MAX, MissingCount);

    res = ScDeInit(&testSc);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
}

void test_SequenceChecker_DuplicateFrames(void)
{
    ScSequenceType testSc;
    uint32_t MissingCount = 0U;
    ScStatusType res      = SC_E_OK;

    res = ScInit(&testSc, UINT32_MAX, UINT32_MAX);
    TEST_ASSERT_EQUAL(SC_E_OK, res);

    res = ScCheckSequence(&testSc, UINT32_MAX);
    TEST_ASSERT_EQUAL(SC_E_SEQUENCE, res);

    res = ScGetMissingCount(&testSc, &MissingCount);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
    TEST_ASSERT_EQUAL(0U, MissingCount);

    res = ScDeInit(&testSc);
    TEST_ASSERT_EQUAL(SC_E_OK, res);
}

void RunAllTests(void)
{
    RUN_TEST(test_SequenceChecker_HappyPath);
    RUN_TEST(test_SequenceChecker_MissingFrames);
    RUN_TEST(test_SequenceChecker_DuplicateFrames);
}

void test_SequenceChecker()
{
    RunAllTests();
}
