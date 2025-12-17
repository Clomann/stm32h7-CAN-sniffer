#include "fdcan_msg_port.h"

#include "CommTypes.h"
#include "FreeRTOS.h"
#include "message_buffer.h"

#include "gpio.h"

static MessageBufferHandle_t CanFrameBuffer;
#define CAN_FRAME_BUFFER_SIZE (1000U * sizeof(FDCAN_ClassicFrameType))
__attribute__((section(".dtcram"))) 
uint8_t MessageBufferStorageArea[CAN_FRAME_BUFFER_SIZE];
static StaticMessageBuffer_t MessageBuffer;
static volatile uint32_t FrameDropCount_FcdanMsgPort = 0;

// STATIC_ASSERT( CAN_FRAME_BUFFER_SIZE >= 2U * sizeof(FDCAN_ClassicFrameType) );

void fdcan_msg_port_receive(FDCAN_ClassicFrameType *frame)
{
    BaseType_t xHigher = pdFALSE;
    size_t bytes_sent;

    bytes_sent = xMessageBufferSendFromISR(CanFrameBuffer, (uint8_t *)frame, sizeof(FDCAN_ClassicFrameType), &xHigher);
    
    if (0 == bytes_sent)
    {
        GPIO_Dbg_On(GPIO_PIN_1);
        GPIO_Dbg_Off(GPIO_PIN_1);
        FrameDropCount_FcdanMsgPort++;
        CanAbs_ErrorHandler();
    }
    else
    {
    }

    portYIELD_FROM_ISR(xHigher); 
}

void fdcan_msg_port_init(void)
{
    FrameDropCount_FcdanMsgPort = 0U;

    CanFrameBuffer = xMessageBufferCreateStatic(
                        CAN_FRAME_BUFFER_SIZE,
                        MessageBufferStorageArea,
                        &MessageBuffer);
    // configASSERT(CanFrameBuffer);
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrameType *dst, uint32_t ticks)
{
    return xMessageBufferReceive(CanFrameBuffer, dst, sizeof(FDCAN_ClassicFrameType), ticks);
}