#include "CanLogBuffer.h"
#include "lwrb/lwrb.h"

static CanLogType CanLogBuf1;
static lwrb_t Rb1;

uint8_t CanLogBuffer_Init()
{
    return lwrb_init(&Rb1, CanLogBuf1.raw, sizeof(CanLogBuf1.raw));
}

uint8_t CanLogBuffer_AddEntry(const CanLogEntryType* entry)
{
    if (sizeof(CanLogEntryType) == lwrb_write(&Rb1, entry, sizeof(CanLogEntryType)))
    {
        return 0;
    }
    
    return 1;
}

uint8_t CanLogBuffer_IsBlockReady(uint8_t *rdy)
{
    uint32_t delta;

    if (Rb1.w_ptr >= Rb1.r_ptr)
        delta = (Rb1.w_ptr - Rb1.r_ptr);
    else
        delta = (uint32_t)( (int32_t)(Rb1.size) - (int32_t)Rb1.r_ptr + (int32_t)Rb1.w_ptr );
        
    *rdy = delta >= BLOCK_SIZE;
    
    return 0;
}

uint8_t CanLogBuffer_ReadNextBlock(uint8_t *data, uint32_t *len)
{
    uint32_t counter;
    const uint32_t EntryCount = (BLOCK_SIZE/ENTRY_SIZE);
    CanLogEntryType Entry;
    // const char TestData[] = {0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe};
    // char Data[16];
    *len = BLOCK_SIZE;

    for (counter = 0U; counter < EntryCount; counter++)
    {
        if ( sizeof(CanLogEntryType) != lwrb_read(&Rb1, &Entry, sizeof(CanLogEntryType)) )
        {
            break;
        }
        else if ( (BLOCK_SIZE - counter * sizeof(CanLogEntryType)) >= sizeof(CanLogEntryType))
        {
            // utohex_custom(TestData, Data, sizeof(TestData)-1);
            memcpy(&data[counter * sizeof(CanLogEntryType)], &Entry, sizeof(CanLogEntryType));
        }
        else
        {
            break;
        }
    }
    
    return 0U;
}

uint32_t get_timestamp_us(const CanLogEntryType* entry)
{
    return ((uint32_t)entry->timestamp_us.msb << 16) | entry->timestamp_us.lsb;
}
