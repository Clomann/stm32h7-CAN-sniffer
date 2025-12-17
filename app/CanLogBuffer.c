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

uint8_t CanLogBuffer_AddEntry(const void * entry, uint32_t entryTotalSize)
{
    lwrb_sz_t free_space;

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
        if (entryTotalSize > space_in_block)
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

        if (entryTotalSize > free_space)
        {
            return 1;
        }

        if (entryTotalSize == lwrb_write(&Rb1, entry, entryTotalSize))
        {
            CanLogBuffer_FrameCount1++;
            return 0;
        }

        return 1;
    }
}

uint8_t CanLogBuffer_FillBlockWithPadding(void)
{
    uint32_t offset;
    uint32_t pad_len;
    lwrb_sz_t free_space;
    lwrb_sz_t linear_space;
    uint32_t remaining;

    offset = (uint32_t)(Rb1.w_ptr % BLOCK_SIZE);
    if (offset == 0U)
    {
        return CANLOG_E_OK;
    }

    pad_len = BLOCK_SIZE - offset;
    free_space = lwrb_get_free(&Rb1);
    linear_space = lwrb_get_linear_block_write_length(&Rb1);

    if ((free_space < pad_len) || (linear_space < pad_len))
    {
        return CANLOG_E_NOT_OK;
    }

    remaining = pad_len;
    while (remaining > 0U)
    {
        uint32_t chunk = (remaining > sizeof(PaddingChunk)) ? sizeof(PaddingChunk) : remaining;

        if (chunk != lwrb_write(&Rb1, PaddingChunk, chunk))
        {
            return CANLOG_E_NOT_OK;
        }

        remaining -= chunk;
    }

    return CANLOG_E_OK;
}

uint8_t CanLogBuffer_IsBlockReady(uint8_t *rdy)
{
    uint32_t skip_to_align;
    lwrb_sz_t full_len;
    lwrb_sz_t linear_len;
    uint32_t to_boundary;

    skip_to_align = (uint32_t)(Rb1.r_ptr % BLOCK_SIZE);
    full_len = lwrb_get_full(&Rb1);
    linear_len = lwrb_get_linear_block_read_length(&Rb1);

    if (skip_to_align != 0U)
    {
        to_boundary = BLOCK_SIZE - skip_to_align;

        if (full_len <= to_boundary)
        {
            *rdy = 0U;
            return 0;
        }

        if (linear_len <= to_boundary)
        {
            *rdy = 0U;
            return 0;
        }

        full_len -= to_boundary;
        linear_len -= to_boundary;
    }

    *rdy = (full_len >= BLOCK_SIZE) && (linear_len >= BLOCK_SIZE);
    
    return 0;
} 

uint8_t CanLogBuffer_UsedSlots(uint8_t *slots)
{
    uint32_t EntriesRead = 0;
    CanLogEntryStackBufferType EntryBuffer;
    CanLogEntryType *entry = (CanLogEntryType *)(&EntryBuffer);
    uint32_t EntryTotalSize = 0;
    uint32_t SkipBytes = 0;
    uint32_t bytes_available;
    uint32_t BytesRead = 0;
    uint32_t EntryMinSize = 0;
    uint32_t EntryMaxSize = 0;
    uint32_t block_offset = 0;
    uint32_t block_remaining = 0;
    uint32_t skip_to_align = 0;

    do 
    {
        bytes_available = lwrb_get_full(&Rb1);
        if (bytes_available <= SkipBytes) {
            break;
        }

        /* If the read pointer starts mid-block, skip padding up to the next block boundary */
        if ((SkipBytes == 0U) && ((Rb1.r_ptr % BLOCK_SIZE) != 0U))
        {
            skip_to_align = BLOCK_SIZE - (uint32_t)(Rb1.r_ptr % BLOCK_SIZE);

            if (bytes_available < skip_to_align)
            {
                break;
            }

            SkipBytes += skip_to_align;
            continue;
        }

        block_offset = (uint32_t)((Rb1.r_ptr + SkipBytes) % BLOCK_SIZE);

        if ((block_offset == 0U) 
            && ((bytes_available - SkipBytes) >= sizeof(CanLogBlockHeaderType)))
        {
            SkipBytes += sizeof(CanLogBlockHeaderType); 
            continue;
        }

        block_remaining = BLOCK_SIZE - block_offset;
        if (block_remaining < sizeof(entry->header))
        {
            if ((bytes_available - SkipBytes) < block_remaining)
            {
                break;
            }

            SkipBytes += block_remaining;
            continue;
        }

        BytesRead = lwrb_peek(&Rb1, SkipBytes, &entry->header, sizeof(entry->header));
        if (BytesRead != sizeof(entry->header))
        {
            break;
        }

        EntryTotalSize = entry->header.total_len;

        if (EntryTotalSize == 0U)
        {
            break;
        }

        EntryMinSize = 0;
        EntryMaxSize = 0;
        switch (entry->header.type)
        {
            case CLB_ENTRY_TYPE_FRAME:
                EntryMinSize = sizeof(CanLogEntryType);
                EntryMaxSize = sizeof(CanLogEntryType) + CANLOG_ENTRY_MAX_DATA_LENGTH;
                break;
            case CLB_ENTRY_TYPE_SYNC:
                EntryMinSize = sizeof(CanLogSyncType);
                EntryMaxSize = EntryMinSize;
                break;
            case CLB_ENTRY_TYPE_MARKER:
            case CLB_ENTRY_TYPE_NONE:
                EntryMinSize = 0;
                EntryMaxSize = 0;
            default:
                break;
        }

        if (EntryTotalSize < EntryMinSize || EntryTotalSize > EntryMaxSize)
        {
            break;
        }

        if ((bytes_available - SkipBytes) < EntryTotalSize)
        {
            break;
        }

        BytesRead = lwrb_peek(&Rb1, SkipBytes, entry, EntryTotalSize);
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
    uint32_t block_available;
    uint32_t pad_len = 0U;
    lwrb_sz_t free_space = 0U;
    uint32_t remaining = 0U;
    lwrb_sz_t linear_len;

    res = CANLOG_E_OK;
    total_len = 0;
    offset = sizeof(CanLogBlockHeaderType);
    *len = 0U;
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

    block_available = lwrb_get_full(&Rb1);
    linear_len = lwrb_get_linear_block_read_length(&Rb1);

    if (block_available > BLOCK_SIZE)
    {
        block_available = BLOCK_SIZE;
    }

    /* Need at least header plus one entry header */
    if (block_available < (sizeof(CanLogBlockHeaderType) + sizeof(CanLogEntryHeaderType)))
    {
        return CANLOG_E_NOT_OK;
    }

    /* Require a contiguous block for direct write-out */
    if (linear_len < BLOCK_SIZE)
    {
        return CANLOG_E_NOT_OK;
    }

    /* If this is a partial block and there is contiguous free space to extend it,
     * pad to the block boundary with 0xFF so the emitted block is fully padded. */
    if (block_available < BLOCK_SIZE)
    {
        pad_len = BLOCK_SIZE - block_available;
        free_space = lwrb_get_free(&Rb1);

        if (free_space < pad_len)
        {
            return CANLOG_E_NOT_OK;
        }

        remaining = pad_len;

        while (remaining > 0U)
        {
            uint32_t chunk = (remaining > sizeof(PaddingChunk)) ? sizeof(PaddingChunk) : remaining;

            if (chunk != lwrb_write(&Rb1, PaddingChunk, chunk))
            {
                return CANLOG_E_NOT_OK;
            }

            remaining -= chunk;
        }

        block_available = BLOCK_SIZE;
    }

    block_ptr = lwrb_get_linear_block_read_address(&Rb1);

    while (CANLOG_E_OK == res)
    {
        if ((block_available - offset) < sizeof(EntryHeader)) {
            break; // Not enough data for another header in this block
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

        if (CLB_ENTRY_TYPE_NONE == entry_type)
        {
            break;
        }

        if ((offset + total_len) > block_available) {
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

    /* After a partial flush the read pointer may sit mid-block. If the buffer
     * is now empty, reset the ring to realign r/w pointers to block start. */
    if ((0U == lwrb_get_full(&Rb1)) && ((Rb1.r_ptr % BLOCK_SIZE) != 0U))
    {
        lwrb_reset(&Rb1);
    }

    return 0;
}
