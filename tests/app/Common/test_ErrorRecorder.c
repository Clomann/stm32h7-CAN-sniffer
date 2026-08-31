#include "test_ErrorRecorder.h"

#include "unity.h"

#include "ErrorRecorder.h"

static void RunAllTests(void);

void test_ErrorRecorder_setUp(void)
{
    ErrorRecorder_Clear();
}

void test_ErrorRecorder_tearDown(void)
{
    ErrorRecorder_Clear();
}

static ErrorContextType make_context(uint32_t value)
{
    ErrorContextType context = {0};

    context.id   = 0x46480000U + value;
    context.code = value;
    context.arg0 = value + 10U;
    context.arg1 = value + 20U;
    context.line = (uint16_t)(100U + value);

    return context;
}

static void test_ErrorRecorder_EmptySnapshot(void)
{
    ErrorRecorderSnapshotType snapshot;

    TEST_ASSERT_EQUAL_UINT8(0U, ErrorRecorder_GetSnapshot(&snapshot));
    TEST_ASSERT_EQUAL_UINT32(0U, snapshot.total_count);
    TEST_ASSERT_EQUAL_UINT8(0U, snapshot.first_valid);
    TEST_ASSERT_EQUAL_UINT8(0U, snapshot.last_valid);
    TEST_ASSERT_EQUAL_UINT8(0U, snapshot.recent_count);
}

static void test_ErrorRecorder_StoresFirstAndLast(void)
{
    ErrorRecorderSnapshotType snapshot;
    ErrorContextType first  = make_context(1U);
    ErrorContextType second = make_context(2U);

    ErrorRecorder_Record(&first);
    ErrorRecorder_Record(&second);

    TEST_ASSERT_EQUAL_UINT8(0U, ErrorRecorder_GetSnapshot(&snapshot));
    TEST_ASSERT_EQUAL_UINT32(2U, snapshot.total_count);
    TEST_ASSERT_EQUAL_UINT8(1U, snapshot.first_valid);
    TEST_ASSERT_EQUAL_UINT8(1U, snapshot.last_valid);
    TEST_ASSERT_EQUAL_UINT32(1U, snapshot.first.sequence);
    TEST_ASSERT_EQUAL_UINT32(first.id, snapshot.first.context.id);
    TEST_ASSERT_EQUAL_UINT32(2U, snapshot.last.sequence);
    TEST_ASSERT_EQUAL_UINT32(second.id, snapshot.last.context.id);
}

static void test_ErrorRecorder_RecentRingIsOldestToNewest(void)
{
    ErrorRecorderSnapshotType snapshot;

    for (uint32_t i = 1U; i <= 12U; i++)
    {
        ErrorContextType context = make_context(i);
        ErrorRecorder_Record(&context);
    }

    TEST_ASSERT_EQUAL_UINT8(0U, ErrorRecorder_GetSnapshot(&snapshot));
    TEST_ASSERT_EQUAL_UINT32(12U, snapshot.total_count);
    TEST_ASSERT_EQUAL_UINT32(2U, snapshot.overwritten_count);
    TEST_ASSERT_EQUAL_UINT8(ERROR_RECORDER_RECENT_COUNT, snapshot.recent_count);
    TEST_ASSERT_EQUAL_UINT32(1U, snapshot.first.sequence);
    TEST_ASSERT_EQUAL_UINT32(12U, snapshot.last.sequence);

    for (uint8_t i = 0U; i < ERROR_RECORDER_RECENT_COUNT; i++)
    {
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i + 3U, snapshot.recent[i].sequence);
        TEST_ASSERT_EQUAL_UINT32(
            0x46480000U + (uint32_t)i + 3U,
            snapshot.recent[i].context.id
        );
    }
}

static void RunAllTests(void)
{
    RUN_TEST(test_ErrorRecorder_EmptySnapshot);
    RUN_TEST(test_ErrorRecorder_StoresFirstAndLast);
    RUN_TEST(test_ErrorRecorder_RecentRingIsOldestToNewest);
}

void test_ErrorRecorder(void)
{
    RunAllTests();
}
