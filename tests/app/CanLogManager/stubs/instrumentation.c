#include "instrumentation.h"

InstrErrorType Instrumentation_SerializeHook(uint8_t *buf, uint32_t *size)
{
    if (buf && size)
    {
        *size = 0;
    }
    return 0;
}

uint64_t FDCAN_GetTimestampHook(void)
{
    static uint64_t ts = 0;
    return ts += 100;
}
