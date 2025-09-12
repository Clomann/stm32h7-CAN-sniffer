#include <string.h>

#include "unity.h"

#include "./CanLogManager/test_CanLogManager.h"

void setUp(void)
{
    test_CanLogManager_setUp();
}

void tearDown(void)
{
    test_CanLogManager_tearDown();
}

int main(void)
{
    UNITY_BEGIN();

    test_CanLogBuffer();

    return UNITY_END();
}
