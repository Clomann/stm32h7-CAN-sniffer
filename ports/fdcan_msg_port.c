#include "fdcan_msg_port.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "message_buffer.h"

#include "gpio.h"
#include "RuntimeChecks.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


#define CAN_FRAME_BUFFER_SIZE (600U * (sizeof(PageType) + sizeof(configMESSAGE_BUFFER_LENGTH_TYPE)) + 1U)
#define CAN_FRAME_PAGE_MAX_ENTRIES   (10U)

typedef struct {
    FDCAN_ClassicFrame frame;
} PageEntryType;

typedef struct {
    uint32_t count;
    PageEntryType frames[CAN_FRAME_PAGE_MAX_ENTRIES];
} PageType;

static MessageBufferHandle_t CanFrameBuffer;
static StaticMessageBuffer_t MessageBuffer;
__attribute__((section(".dtcram"))) 
uint8_t MessageBufferStorageArea[CAN_FRAME_BUFFER_SIZE];

// STATIC_ASSERT( CAN_FRAME_BUFFER_SIZE >= 2U * sizeof(FDCAN_ClassicFrame) );

static inline void AddEntryToPage(PageType *page, const FDCAN_ClassicFrame *frame)
{
    // page->frames[page->count].size = sizeof(FDCAN_ClassicFrame);
    memcpy(&page->frames[page->count].frame, frame, sizeof(FDCAN_ClassicFrame));
    page->count++;
}

void fdcan_msg_port_receive(FDCAN_ClassicFrame *frame)
{
    BaseType_t xHigher = pdFALSE;
    size_t bytes_sent = 0;
    static PageType Page = {
        .count = 0
    };

    if (Page.count >= CAN_FRAME_PAGE_MAX_ENTRIES)
    {
        bytes_sent = xMessageBufferSendFromISR(CanFrameBuffer, (uint8_t *)&Page, sizeof(Page), &xHigher);
        
        Page.count = 0;

        if (bytes_sent == 0)
        {
            FcdanMsgPort_FrameDropCount++;
            CanAbs_ErrorHandler();
            /* page still full; drop this frame */
            portYIELD_FROM_ISR(xHigher);
            return;
        }
        else
        {
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

    FcdanMsgPort_FrameDropCount = 0U;

    CanFrameBuffer = xMessageBufferCreateStatic(
                        CAN_FRAME_BUFFER_SIZE,
                        MessageBufferStorageArea,
                        &MessageBuffer);

    NeededSize = sizeof(MessageBufferStorageArea);
    ActualSize = xMessageBufferSpaceAvailable(CanFrameBuffer);

    if (NeededSize - 1U > ActualSize)
    {
        (void) NeededSize;
        (void) ActualSize;
    }
    // configASSERT(CanFrameBuffer != NULL);
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrame **dst, uint32_t milliSeconds)
{
    size_t BytesReceived = 0U;
    static uint32_t Index = 0;
    static PageType Page = {0};
    
    if (Index == 0)
    {
        BytesReceived = xMessageBufferReceive(CanFrameBuffer, &Page, sizeof(Page), pdMS_TO_TICKS(milliSeconds));
        
        if (BytesReceived > 0)
        {
            Index = Page.count;
        }
        else 
        {
            Index = 0;
        }
    }
    
    if (Index > 0)
    {
        *dst = &Page.frames[Page.count - Index].frame;
        BytesReceived = sizeof(Page.frames[Page.count - Index].frame); // Page.frames[Page.count - Index].size;
        Index--;
    }   

    return BytesReceived;
}
