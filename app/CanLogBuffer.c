#include "CanLogBuffer.h"
#include "lwrb/lwrb.h"

__attribute__((section(".ram_d2")))
static uint8_t CanLogBuf1[LOG_BUFFER_SIZE] ;
static lwrb_t Rb1;
static uint8_t EpochCount = 0;
static uint8_t BlockCount = 0;

extern uint64_t CanLogBufferFrameCount1;
extern uint64_t CanLogBufferFrameCount2;

uint8_t CanLogBuffer_Init()
{
    return lwrb_init(&Rb1, CanLogBuf1, sizeof(CanLogBuf1));
}

void CanLogBuffer_SetEpochCount(uint8_t epoch)
{
    EpochCount = epoch;
}

uint8_t CanLogBuffer_AddClassicCanEntry(const CanLogClassicCanEntryType * entry)
{
    if (sizeof(CanLogClassicCanEntryType) == lwrb_write(&Rb1, entry, sizeof(CanLogClassicCanEntryType)))
    {
        CanLogBufferFrameCount1++;
        
        return 0;
    }
    
    return 1;
}

uint8_t CanLogBuffer_AddFdCanEntry(const CanLogFdcanCanEntryType * entry)
{
    if (sizeof(CanLogFdcanCanEntryType) == lwrb_write(&Rb1, entry, sizeof(CanLogFdcanCanEntryType)))
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

uint8_t CanLogBuffer_UsedSlots(uint8_t *slots)
{
    uint32_t delta;

    if (Rb1.w_ptr >= Rb1.r_ptr)
        delta = (Rb1.w_ptr - Rb1.r_ptr);
    else
        delta = (uint32_t)( (int32_t)(Rb1.size) - (int32_t)Rb1.r_ptr + (int32_t)Rb1.w_ptr );
        
    *slots = delta / sizeof(CanLogClassicCanEntryType);
    
    return 0;
}

uint8_t CanLogBuffer_ReadNextBlock(uint8_t *data, uint32_t *len)
{
    uint8_t res;
    uint16_t total_len;
    uint8_t entry_type;
    uint32_t offset;
    CanLogBlockHeaderType BlockHeader;
    CanLogEntryHeaderType EntryHeader;

    res = CANLOG_E_OK;
    total_len = 0;
    offset = sizeof(CanLogBlockHeaderType);
    *len = BLOCK_SIZE;

    while (CANLOG_E_OK == res)
    {
        if (lwrb_get_full(&Rb1) < sizeof(EntryHeader)) {
            break; // Not enough data
        }

        if (lwrb_peek(&Rb1, 0, &EntryHeader, sizeof(EntryHeader)) != sizeof(EntryHeader)) {
            break; // Error
        }

        total_len = EntryHeader.total_len;
        entry_type = EntryHeader.type;

        if (CANLOG_UNDEFINED_TYPE == entry_type)
        {
            res = CANLOG_E_OK;
            break;
        }

        if ((offset + total_len) > BLOCK_SIZE) {
            break; // Output block full
        }

        if (lwrb_get_full(&Rb1) < total_len) {
            break;
        }

        if (lwrb_read(&Rb1, &data[offset], total_len) != total_len) {
            break;
        }

        CanLogBufferFrameCount2++;
        offset += total_len;
    }
    
    if (offset != sizeof(BlockHeader))
    {
        if (offset < BLOCK_SIZE) {
            memset(&data[offset], 0xFF, BLOCK_SIZE - offset);
        }
    
        BlockHeader.block_fill = offset;
        BlockHeader.block_size = BLOCK_SIZE;
        BlockHeader.header_size = sizeof(BlockHeader);
        BlockHeader.version = CANLOG_VERSION;
        BlockHeader.cnt = BlockCount++;
        BlockHeader.epoch = EpochCount;

        memcpy(data, &BlockHeader, sizeof(BlockHeader));
    }
    else
    {
        res = CANLOG_E_NOT_OK;
    }
    
    return res;
}
