#include "fdcan_msg_port.h"

#include "CommTypes.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "message_buffer.h"
#include "memory_sections.h"

#include "gpio.h"
#include "RuntimeChecks.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define CAN_FRAME_BUFFER_ELEMENTS_COUNT 600U
#define CAN_FRAME_BUFFER_ENTRY_SIZE                                            \
    (sizeof(PageType) + sizeof(configMESSAGE_BUFFER_LENGTH_TYPE))
#define CAN_FRAME_BUFFER_SIZE                                                  \
    (CAN_FRAME_BUFFER_ELEMENTS_COUNT * CAN_FRAME_BUFFER_ENTRY_SIZE + 1U)
#define CAN_FRAME_PAGE_MAX_ENTRIES (10U)

typedef struct
{
    FDCAN_ClassicFrameType frame;
} PageEntryType;

typedef struct
{
    uint32_t count;
    PageEntryType frames[CAN_FRAME_PAGE_MAX_ENTRIES];
} PageType;

static MessageBufferHandle_t CanFrameBuffer;
static StaticMessageBuffer_t MessageBuffer;
uint8_t MessageBufferStorageArea[CAN_FRAME_BUFFER_SIZE] RAM_DTC_SECTION;
static uint32_t CanFrameBufferHighWaterBytes = 0U;

__attribute__((weak)) void
FdcanMsgPort_InstrumentationUsedBytesHook(uint32_t used_bytes)
{
    (void)used_bytes;
}

/**
 * @brief CAN frame page buffer shared between ISR and task context
 * 
 * Accumulates incoming frames into pages. Access synchronized via:
 * - ISR: adds frames, sends full pages
 * - Task: flushes partial pages (requires critical section)
 */
static PageType Page = {.count = 0};

// STATIC_ASSERT( CAN_FRAME_BUFFER_SIZE >= 2U * sizeof(FDCAN_ClassicFrameType) );

static void fdcan_msg_port_update_highwater(void)
{
    size_t space;
    size_t capacity;
    size_t used;

    if (CanFrameBuffer == NULL)
    {
        return;
    }

    capacity = (CAN_FRAME_BUFFER_SIZE > 0U) ? (CAN_FRAME_BUFFER_SIZE - 1U) : 0U;
    space    = xMessageBufferSpacesAvailable(CanFrameBuffer);
    if (space > capacity)
    {
        return;
    }

    used = capacity - space;
    FdcanMsgPort_InstrumentationUsedBytesHook((uint32_t)used);

    if (used > CanFrameBufferHighWaterBytes)
    {
        CanFrameBufferHighWaterBytes = (uint32_t)used;
    }
}

static inline void
AddEntryToPage(PageType *page, const FDCAN_ClassicFrameType *frame)
{
    // page->frames[page->count].size = sizeof(FDCAN_ClassicFrameType);
    memcpy(
        &page->frames[page->count].frame,
        frame,
        sizeof(FDCAN_ClassicFrameType)
    );
    page->count++;
}

void fdcan_msg_port_receive(FDCAN_ClassicFrameType *frame)
{
    BaseType_t xHigher = pdFALSE;
    size_t bytes_sent  = 0;

    if (Page.count >= CAN_FRAME_PAGE_MAX_ENTRIES)
    {
        bytes_sent = xMessageBufferSendFromISR(
            CanFrameBuffer,
            (uint8_t *)&Page,
            sizeof(Page),
            &xHigher
        );
        if (bytes_sent > 0)
        {
            Page.count = 0;
            fdcan_msg_port_update_highwater();
        }
        else
        {
            FcdanMsgPort_FrameDropCount += 1U;
            CanAbs_ErrorHandler();
            /* page still full; drop this frame */
            portYIELD_FROM_ISR(xHigher);
            return;
        }
    }

    if (Page.count < CAN_FRAME_PAGE_MAX_ENTRIES)
    {
        AddEntryToPage(&Page, frame);
    }

    portYIELD_FROM_ISR(xHigher);
}

void fdcan_msg_port_init(void)
{
    volatile size_t NeededSize;
    volatile size_t ActualSize;

    FcdanMsgPort_FrameDropCount  = 0U;
    CanFrameBufferHighWaterBytes = 0U;

    CanFrameBuffer = xMessageBufferCreateStatic(
        CAN_FRAME_BUFFER_SIZE,
        MessageBufferStorageArea,
        &MessageBuffer
    );

    NeededSize = sizeof(MessageBufferStorageArea);
    ActualSize = xMessageBufferSpaceAvailable(CanFrameBuffer);

    if (NeededSize - 1U > ActualSize)
    {
        (void)NeededSize;
        (void)ActualSize;
    }
    // configASSERT(CanFrameBuffer != NULL);
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrameType **dst, uint32_t milliSeconds)
{
    size_t BytesReceived  = 0U;
    static uint32_t Index = 0;
    static PageType Page  = {0};

    if (Index == 0)
    {
        BytesReceived = xMessageBufferReceive(
            CanFrameBuffer,
            &Page,
            sizeof(Page),
            pdMS_TO_TICKS(milliSeconds)
        );

        if (BytesReceived > 0)
        {
            Index = Page.count;
            fdcan_msg_port_update_highwater();
        }
        else
        {
            Index = 0;
        }
    }

    if (Index > 0)
    {
        *dst          = &Page.frames[Page.count - Index].frame;
        BytesReceived = sizeof(Page.frames[Page.count - Index].frame
        ); // Page.frames[Page.count - Index].size;
        Index--;
    }

    return BytesReceived;
}

void fdcan_msg_port_flush(void)
{
    size_t bytes_sent;

    if (Page.count > 0)
    {
        bytes_sent = xMessageBufferSend(
            CanFrameBuffer,
            (uint8_t *)&Page,
            sizeof(Page),
            portMAX_DELAY
        );
        if (bytes_sent > 0)
        {
            Page.count = 0;
            fdcan_msg_port_update_highwater();
        }
        else
        {
            FcdanMsgPort_FrameDropCount += Page.count;
            CanAbs_ErrorHandler();
            return;
        }
    }
}

uint8_t fdcan_msg_port_get_highwater_bytes(uint32_t *bytes)
{
    if (bytes == NULL)
    {
        return 1U;
    }

    *bytes = CanFrameBufferHighWaterBytes;
    return 0U;
}

uint32_t fdcan_msg_port_get_capacity_bytes(void)
{
    if (CAN_FRAME_BUFFER_SIZE > 0U)
    {
        return (uint32_t)(CAN_FRAME_BUFFER_SIZE - 1U);
    }

    return 0U;
}
