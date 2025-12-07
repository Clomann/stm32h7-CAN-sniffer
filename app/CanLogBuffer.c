#include "CanLogBuffer.h"
#include "lwrb/lwrb.h"
#include "RuntimeChecks.h"
#include <stdint.h>
#include <string.h>

__attribute__((section(".ram_d1")))
static uint8_t CanLogBuf1[LOG_BUFFER_SIZE] ;
static lwrb_t Rb1;
static uint8_t EpochCount = 0;
static uint8_t BlockCount = 0;
static const uint8_t PaddingChunk[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static const uint8_t HeaderPlaceholder[sizeof(CanLogBlockHeaderType)] = {0};

uint8_t CanLogBuffer_Init()
{
    return lwrb_init(&Rb1, CanLogBuf1, sizeof(CanLogBuf1));
}

void CanLogBuffer_SetEpochCount(uint8_t epoch)
{
    EpochCount = epoch;
}

uint8_t CanLogBuffer_AddEntry(const CanLogEntryType * entry)
{
    uint32_t EntryTotalSize;
    lwrb_sz_t free_space;

    EntryTotalSize = entry->header.total_len;

    while (1)
    {
        uint32_t offset         = (uint32_t)(Rb1.w_ptr % BLOCK_SIZE);
        uint32_t space_in_block = BLOCK_SIZE - offset;

        free_space = lwrb_get_free(&Rb1);

        /* Reserve header space at the start of each block */
        if (0U == offset)
        {
            if (free_space < sizeof(HeaderPlaceholder))
            {
                return 1;
            }

            if (sizeof(HeaderPlaceholder) != lwrb_write(&Rb1, HeaderPlaceholder, sizeof(HeaderPlaceholder)))
            {
                return 1;
            }

            continue; /* offset changed, re-evaluate with new pointer */
        }

        /* If entry would cross the block boundary, pad to the end of this block */
        if (EntryTotalSize > space_in_block)
        {
            uint32_t padding_to_next = space_in_block;

            if (padding_to_next > free_space)
            {
                return 1;
            }

            while (padding_to_next > 0U)
            {
                uint32_t chunk = (padding_to_next > sizeof(PaddingChunk)) ? sizeof(PaddingChunk) : padding_to_next;

                if (chunk != lwrb_write(&Rb1, PaddingChunk, chunk))
                {
                    return 1;
                }

                padding_to_next -= chunk;
            }

            continue; /* start again at next block (header will be reserved) */
        }

        if (EntryTotalSize > free_space)
        {
            return 1;
        }

        if (EntryTotalSize == lwrb_write(&Rb1, entry, EntryTotalSize))
        {
            CanLogBuffer_FrameCount1++;
            return 0;
        }

        return 1;
    }
}

uint8_t CanLogBuffer_IsBlockReady(uint8_t *rdy)
{
    uint32_t delta;
    uint32_t skip_to_align;
    lwrb_sz_t linear_len;

    skip_to_align = (uint32_t)(Rb1.r_ptr % BLOCK_SIZE);
    if (skip_to_align != 0U)
    {
        skip_to_align = BLOCK_SIZE - skip_to_align;
    }

    if (Rb1.w_ptr >= Rb1.r_ptr)
        delta = (Rb1.w_ptr - Rb1.r_ptr);
    else
        delta = (uint32_t)( (int32_t)(Rb1.size) - (int32_t)Rb1.r_ptr + (int32_t)Rb1.w_ptr );
        
    if (delta <= skip_to_align)
    {
        *rdy = 0U;
    }
    else
    {
        delta -= skip_to_align;
        linear_len = lwrb_get_linear_block_read_length(&Rb1);

        if (linear_len < skip_to_align)
        {
            *rdy = 0U;
        }
        else
        {
            linear_len -= skip_to_align;
            *rdy = (delta >= BLOCK_SIZE) && (linear_len >= BLOCK_SIZE);
        }
    }
    
    return 0;
} 

uint8_t CanLogBuffer_UsedSlots(uint8_t *slots)
{
    uint32_t EntriesRead = 0;
    CanLogEntryAccessorType Entry;
    uint32_t EntryTotalSize = 0;
    uint32_t SkipBytes = 0;
    uint32_t bytes_available;
    uint32_t BytesRead = 0;

    do 
    {
        bytes_available = lwrb_get_full(&Rb1);
        if (bytes_available <= SkipBytes) {
            break;
        }

        BytesRead = lwrb_peek(&Rb1, SkipBytes, &Entry.accessor.header, sizeof(Entry.accessor.header));
        if (BytesRead != sizeof(Entry.accessor.header))
        {
            break;
        }

        EntryTotalSize = Entry.accessor.header.total_len;

        if (EntryTotalSize < sizeof(Entry.accessor.header) || EntryTotalSize > sizeof(Entry))
        {
            break;
        }

        if ((bytes_available - SkipBytes) < EntryTotalSize)
        {
            break;
        }

        BytesRead = lwrb_peek(&Rb1, SkipBytes, &Entry.accessor, EntryTotalSize);
        if (BytesRead != EntryTotalSize)
        {
            break;
        }

        SkipBytes += EntryTotalSize;        
        EntriesRead++;
    } while (1);

    *slots = EntriesRead;
    
    return 0;
}

uint8_t CanLogBuffer_ReadNextBlock(uint8_t **data, uint32_t *len, uint32_t *frame_count)
{
    volatile uint8_t res;
    uint32_t total_len;
    uint8_t entry_type;
    uint32_t offset;
    uint32_t BytesRead = 0;
    uint32_t skip_to_align;
    CanLogBlockHeaderType BlockHeader;
    CanLogEntryHeaderType EntryHeader;
    uint8_t *block_ptr;

    res = CANLOG_E_OK;
    total_len = 0;
    offset = sizeof(CanLogBlockHeaderType);
    *len = BLOCK_SIZE;
    *frame_count = 0U;

    skip_to_align = (uint32_t)(Rb1.r_ptr % BLOCK_SIZE);
    if (skip_to_align != 0U)
    {
        skip_to_align = BLOCK_SIZE - skip_to_align;

        if (lwrb_get_full(&Rb1) < skip_to_align)
        {
            return CANLOG_E_NOT_OK;
        }

        lwrb_skip(&Rb1, skip_to_align);
    }

    if ((lwrb_get_full(&Rb1) < BLOCK_SIZE) ||
        (lwrb_get_linear_block_read_length(&Rb1) < BLOCK_SIZE))
    {
        return CANLOG_E_NOT_OK;
    }

    block_ptr = lwrb_get_linear_block_read_address(&Rb1);

    while (CANLOG_E_OK == res)
    {
        if (lwrb_get_full(&Rb1) < sizeof(EntryHeader)) {
            break; // Not enough data
        }

        BytesRead = lwrb_peek(&Rb1, offset, &EntryHeader, sizeof(EntryHeader));
        if (BytesRead != sizeof(EntryHeader)) {
            break; // Error
        }

        total_len = EntryHeader.total_len;
        entry_type = EntryHeader.type;

        /* Bail out on malformed/degenerate entries to avoid an infinite loop. */
        if ((total_len < sizeof(EntryHeader)) || (total_len > BLOCK_SIZE))
        {
            break;
        }

        if (CANLOG_UNDEFINED_TYPE == entry_type)
        {
            break;
        }

        if ((offset + total_len) > BLOCK_SIZE) {
            break; // Output block full
        }

        offset += total_len;
        (*frame_count)++;
    }
    
    if (offset != sizeof(BlockHeader))
    {
        BlockHeader.block_fill = offset;
        BlockHeader.block_size = BLOCK_SIZE;
        BlockHeader.header_size = sizeof(BlockHeader);
        BlockHeader.version = CANLOG_VERSION;
        BlockHeader.cnt = BlockCount++;
        BlockHeader.epoch = EpochCount;

        BlockHeader.ingress_frames = CanLogBuffer_FrameCount1;
        BlockHeader.frame_count = *frame_count;

        memcpy(block_ptr, &BlockHeader, sizeof(BlockHeader));

        *data = block_ptr;
        *len = BLOCK_SIZE;
    }
    else
    {
        res = CANLOG_E_NOT_OK;
    }

    (void) res;
    
    return res;
}

uint8_t CanLogBuffer_Consume(uint32_t len, uint32_t frame_count)
{
    lwrb_skip(&Rb1, len);
    CanLogBuffer_FrameCount2 += frame_count;
    return 0;
}
