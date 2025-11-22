#include "CanAbs.h"
#include "buffers.h"
#include "fdcan.h"

#include <stdint.h>

/**
 * @brief Hook called at CAN ISR entry for measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanAbs_InstrumentationIsrStartHook(void) {}

/**
 * @brief Hook called at CAN ISR exit for measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanAbs_InstrumentationIsrEndHook(void) {}

/* CAN 1 */

static CommDriver Fdcan1Driver;
static CommDriverConfigType Fdcan1Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_1
};
static FDCAN_ClassicFrame Fdcan1RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
static FDCAN_ClassicFrame Fdcan1TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0};
static RingBuffer Fdcan1RxRingBuffer = {
    .startAddress = &Fdcan1RxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan1RxFrameBuffer) / sizeof(Fdcan1RxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan1RxFrameBuffer[0]),
    .stride = sizeof(Fdcan1RxFrameBuffer[0]),
    .isFull = false
};

static RingBuffer Fdcan1TxRingBuffer = {
    .startAddress = &Fdcan1TxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan1TxFrameBuffer) / sizeof(Fdcan1TxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan1TxFrameBuffer[0]),
    .stride = sizeof(Fdcan1TxFrameBuffer[0]),
    .isFull = false
};

/* CAN 2 */

static CommDriver Fdcan2Driver;
static CommDriverConfigType Fdcan2Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_2
};
static FDCAN_ClassicFrame Fdcan2RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
static FDCAN_ClassicFrame Fdcan2TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0};
static RingBuffer Fdcan2RxRingBuffer = {
    .startAddress = &Fdcan2RxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan2RxFrameBuffer) / sizeof(Fdcan2RxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan2RxFrameBuffer[0]),
    .stride = sizeof(Fdcan2RxFrameBuffer[0]),
    .isFull = false
};

static RingBuffer Fdcan2TxRingBuffer = {
    .startAddress = &Fdcan2TxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan2TxFrameBuffer) / sizeof(Fdcan2TxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan2TxFrameBuffer[0]),
    .stride = sizeof(Fdcan2TxFrameBuffer[0]),
    .isFull = false
};

/* Private functions */

/**
 * Little helper function to make ioctl function call a bit prettier.
 */
static inline int can_ioctl(struct CommDriver *dev, int cmd, void *arg) {
    return dev->interface->ioctl(dev, cmd, arg);
}

int CanAbs_Init(CommDriver *dev, CommDriverConfigType *cfg, uint8_t *tx, uint8_t *rx)
{
    unsigned int res = COMM_SUCCESS;

    dev->protocol = DRIVER_FDCAN;

    (void)CommManager_Init(dev, (const void *)cfg, sizeof(CommDriverConfigType), tx, rx);

	if (dev->interface->init(dev) != COMM_SUCCESS)
	{
	  res = 1;
	}

    return res;
}

int CanAbs_Receive(CommDriver *dev, FDCAN_ClassicFrame *frame)
{
    return ring_buffer_pop((RingBuffer *)dev->RxFrameBuffer, (void*)frame);
}

comm_status_t CanAbs_Send(CommDriver *dev, FDCAN_Message *msg)
{
    return dev->interface->send(dev, msg);
}

comm_status_t CanAbs_Start(CommDriver *dev)
{
    uint32_t val = 1;
    return dev->interface->ioctl(dev, CANABS_IOCTL_CMD_START, &val);
}

comm_status_t CanAbs_Stop(CommDriver *dev)
{
    uint32_t val = 1;
    return dev->interface->ioctl(dev, CANABS_IOCTL_CMD_STOP, &val);
}

comm_status_t CanAbs_SetBaudrate(CommDriver *dev, uint32_t baudrate)
{
    return dev->interface->ioctl(dev, CANABS_IOCTL_CMD_SET_BAUDRATE, &baudrate);
}

comm_status_t CanAbs_SetMode(CommDriver *dev, uint32_t mode)
{
    return dev->interface->ioctl(dev, CANABS_IOCTL_CMD_SET_MODE, &mode);
}

comm_status_t CanAbs_CreateMessage_Standard(
    FDCAN_Message *pMsg, 
    uint32_t id, 
    uint8_t *pData, 
    uint32_t length)
{

	{
		pMsg->can_id = id;
		pMsg->isExtendedId = 0U;
		pMsg->frame_type = 0U;
		pMsg->msgMarker = 0U;

		pMsg->msgBase.dir = DRIVER_MSGDIRECTION_TX;
		pMsg->msgBase.isMmultiframe = 0;
		pMsg->msgBase.length = length;
		pMsg->msgBase.payload = pData;
		pMsg->msgBase.protocol = DRIVER_FDCAN;
	}

	return COMM_SUCCESS;
}

void NotifyConsumerTask(void)
{
    CanAbs_RxNotificationCallback();
}

/**
 * @brief Reconstructs a full 64-bit timestamp from a 16-bit hardware timer value.
 * 
 * Handles timer wraparound by determining which epoch the hardware timestamp belongs to.
 * Used in CAN ISR where delay between frame capture and ISR execution may cause wraparound.
 * 
 * The algorithm compares the hardware timestamp with the current timer position:
 * - Large positive difference (> Period/2): Frame from previous epoch (wrapped)
 * - Large negative difference (< -Period/2): Frame from next epoch (rare edge case)
 * - Otherwise: Frame from current epoch (normal case)
 * 
 * @param[in] hardware_timestamp  16-bit timestamp captured by CAN peripheral at frame arrival
 * @param[in] global_timestamp    Current system timestamp read in ISR
 * 
 * @return Full 64-bit absolute timestamp of frame arrival
 * 
 * @note ISR latency must be < Period/2 for correct epoch detection
 * @note Returns hardware_timestamp unchanged if Period is 0
 */
static uint64_t ReconstructFullTimestamp(uint64_t hardware_timestamp, uint64_t global_timestamp)
{
    uint64_t Period;
    uint64_t current_timer_value;
    uint64_t epoch_count;
    int64_t time_diff;

    Period = FDCAN_GetTimerPeriodHook();
    
    if (Period == 0) 
    {
        return hardware_timestamp;
    }
   
    // Extract current timer value and epoch count based on actual timer period
    current_timer_value = global_timestamp % Period;
    epoch_count = global_timestamp / Period;


    time_diff = (int64_t)hardware_timestamp - (int64_t)current_timer_value;

    // Determine if frame arrived in previous epoch
    if (time_diff > (int64_t)(Period / 2)) {
        epoch_count--;
    }
    // Handle case where frame might be from next epoch
    else if (time_diff < -(int64_t)(Period / 2)) {
        epoch_count++;
    }

    // Reconstruct and return full timestamp
    return ((uint64_t)epoch_count * Period) + hardware_timestamp;
}

/**
  * @brief  Rx FIFO 0 callback.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  RxFifo0ITs: indicates which Rx FIFO 0 interrupts are signalled.
  *                     This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
  * @retval None
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_ClassicFrame NewFrame;
    uint32_t frames_processed = 0;
    uint64_t GlobalTimestamp;
    uint64_t HardwareTimestamp;
    const uint32_t MAX_FRAMES_PER_ISR = FDCAN_RAM_RX_ELEMENTS / 2U;

    CanAbs_InstrumentationIsrStartHook();

    if (FDCAN_1 == hfdcan->Instance)
    {
        while ( (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0) && 
               (frames_processed < MAX_FRAMES_PER_ISR) )
        {
            if (Fdcan1Driver.interface->read(&Fdcan1Driver, (void*)&NewFrame, 8u, RxFifo0ITs) == COMM_SUCCESS)
            {
                NewFrame.channel = 1;
                GlobalTimestamp = FDCAN_GetMostRecentInterruptTimestamp(&Fdcan1Driver);
                HardwareTimestamp = CANABS_ConvertCountToTimestampHook(NewFrame.timestamp);
                NewFrame.timestamp = ReconstructFullTimestamp(HardwareTimestamp, GlobalTimestamp);

                ring_buffer_put((RingBuffer *)Fdcan1Driver.RxFrameBuffer, (void*)&NewFrame);
                if (((RingBuffer *)Fdcan1Driver.RxFrameBuffer)->isFull)
                {
                    CanAbs_ErrorHandler();
                }
                frames_processed++;

                CanAbs_FrameCount++;
            }
        }
    }
    else if (FDCAN_2 == hfdcan->Instance)
    {
        while ( (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0) && 
               (frames_processed < MAX_FRAMES_PER_ISR) )
        {
            if (Fdcan2Driver.interface->read(&Fdcan2Driver, (void*)&NewFrame, 8u, RxFifo0ITs) == COMM_SUCCESS)
            {
                NewFrame.channel = 2;
                GlobalTimestamp = FDCAN_GetMostRecentInterruptTimestamp(&Fdcan2Driver);
                HardwareTimestamp = CANABS_ConvertCountToTimestampHook(NewFrame.timestamp);
                NewFrame.timestamp = ReconstructFullTimestamp(HardwareTimestamp, GlobalTimestamp);

                ring_buffer_put((RingBuffer *)Fdcan2Driver.RxFrameBuffer, (void*)&NewFrame);
                if (((RingBuffer *)Fdcan2Driver.RxFrameBuffer)->isFull)
                {
                    CanAbs_ErrorHandler();
                }
                frames_processed++;

                CanAbs_FrameCount++;
            }
        }
    }
    
    if (frames_processed > 0) {
        NotifyConsumerTask();
    }

    CanAbs_InstrumentationIsrEndHook();
}

/* Public functions ================================================== */

/* CAN 1 */
comm_status_t CanAbs_Init_Can1(uint32_t baudrate)
{
    comm_status_t res;

    res = CanAbs_Init(&Fdcan1Driver, &Fdcan1Config, (uint8_t *)&Fdcan1TxRingBuffer, (uint8_t *)&Fdcan1RxRingBuffer);

    if (0 == COMM_SUCCESS)
    {
        Fdcan1Driver.interface->ioctl(&Fdcan1Driver, CANABS_IOCTL_CMD_SET_BAUDRATE, (void *)&baudrate);
    }

    return res;
}

comm_status_t CanAbs_Send_Can1(FDCAN_Message *msg)
{
    return CanAbs_Send(&Fdcan1Driver, msg);
}

comm_status_t CanAbs_Receive_Can1(FDCAN_ClassicFrame *frame)
{
    return CanAbs_Receive(&Fdcan1Driver, frame);
}

comm_status_t CanAbs_Start_Can1()
{
    return CanAbs_Start(&Fdcan1Driver);
}

comm_status_t CanAbs_Stop_Can1()
{
    return CanAbs_Stop(&Fdcan1Driver);
}

comm_status_t CanAbs_SetBaudrate_Can1(uint32_t baudrate)
{
    return CanAbs_SetBaudrate(&Fdcan1Driver, baudrate);
}

comm_status_t CanAbs_SetMode_Can1(uint32_t mode)
{
    return CanAbs_SetMode(&Fdcan1Driver, mode);
}

comm_status_t CanAbs_IsStateOff_Can1(bool * isOff)
{
    *isOff = Fdcan1Driver.state == DRIVER_STATE_OFF;
    return COMM_SUCCESS;
}

/* CAN 2 */

comm_status_t CanAbs_Init_Can2(uint32_t baudrate)
{
    comm_status_t res;

    res = CanAbs_Init(&Fdcan2Driver, &Fdcan2Config, (uint8_t *)&Fdcan2TxRingBuffer, (uint8_t *)&Fdcan2RxRingBuffer);

    if (0 == COMM_SUCCESS)
    {
        Fdcan2Driver.interface->ioctl(&Fdcan2Driver, CANABS_IOCTL_CMD_SET_BAUDRATE, (void *)&baudrate);
    }

    return res;
}

comm_status_t CanAbs_Send_Can2(FDCAN_Message *msg)
{
    return CanAbs_Send(&Fdcan2Driver, msg);
}

comm_status_t CanAbs_Receive_Can2(FDCAN_ClassicFrame *frame)
{
    return CanAbs_Receive(&Fdcan2Driver, frame);
}

comm_status_t CanAbs_Start_Can2()
{
    return CanAbs_Start(&Fdcan2Driver);
}

comm_status_t CanAbs_Stop_Can2()
{
    return CanAbs_Stop(&Fdcan2Driver);
}

comm_status_t CanAbs_SetBaudrate_Can2(uint32_t baudrate)
{
    return CanAbs_SetBaudrate(&Fdcan2Driver, baudrate);
}

comm_status_t CanAbs_SetMode_Can2(uint32_t mode)
{
    return CanAbs_SetMode(&Fdcan2Driver, mode);
}

comm_status_t CanAbs_IsStateOff_Can2(bool * isOff)
{
    *isOff = Fdcan2Driver.state == DRIVER_STATE_OFF;
    return COMM_SUCCESS;
}

__attribute__((weak))
void  CanAbs_ErrorHandler(void)
{
    ;
}
