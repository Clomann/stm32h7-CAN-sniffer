#include "fdcan_msg_port.h"

#include "FreeRTOS.h"
#include "message_buffer.h"

static MessageBufferHandle_t CanFrameBuffer;
#define CAN_FRAME_BUFFER_SIZE    (50U * sizeof(FDCAN_ClassicFrame))
uint8_t MessageBufferStorageArea[CAN_FRAME_BUFFER_SIZE];
static StaticMessageBuffer_t MessageBuffer;

// STATIC_ASSERT( CAN_FRAME_BUFFER_SIZE >= 2U * sizeof(FDCAN_ClassicFrame) );

void fdcan_msg_port_receive(FDCAN_ClassicFrame *frame)
{
    BaseType_t xHigher = pdFALSE;

    xMessageBufferSendFromISR(CanFrameBuffer, (uint8_t *)frame, sizeof(FDCAN_ClassicFrame), &xHigher);
    
    portYIELD_FROM_ISR(xHigher); 
}

void fdcan_msg_port_init(void)
{
    CanFrameBuffer = xMessageBufferCreateStatic(
                        CAN_FRAME_BUFFER_SIZE,
                        MessageBufferStorageArea,
                        &MessageBuffer);
    // configASSERT(CanFrameBuffer);
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrame *dst, uint32_t ticks)
{
    return xMessageBufferReceive(CanFrameBuffer, dst, sizeof(FDCAN_ClassicFrame), ticks);
}