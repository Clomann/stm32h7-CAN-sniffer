#include <string.h>

#include "unity.h"

#include "./CanLogManager/test_CanLogManager.h"
#include "Common/test_SequenceChecker.h"

void setUp(void)
{
    test_CanLogManager_setUp();
    test_SequenceChecker_setUp();
}

void tearDown(void)
{
    test_CanLogManager_tearDown();
    test_SequenceChecker_tearDown();
}

int main(void)
{
    UNITY_BEGIN();

    test_CanLogBuffer();
    test_SequenceChecker();

    return UNITY_END();
}
