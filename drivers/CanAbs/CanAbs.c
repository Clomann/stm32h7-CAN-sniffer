#include "CanAbs.h"
#include "CommTypes.h"
#include "buffers.h"
#include "fdcan.h"

#include <stdint.h>
#include "RuntimeChecks.h"
#include "stm32h745xx.h"

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
static uint64_t LastHardwareTimestampCan1 = 0;

static CommDriver Fdcan1Driver;
static CommDriverConfigType Fdcan1Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_1
};
static FDCAN_ClassicFrameType Fdcan1RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
static FDCAN_ClassicFrameType Fdcan1TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0};
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
static uint64_t LastHardwareTimestampCan2 = 0;

static CommDriver Fdcan2Driver;
static CommDriverConfigType Fdcan2Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_2
};
static FDCAN_ClassicFrameType Fdcan2RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
static FDCAN_ClassicFrameType Fdcan2TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0};
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

int CanAbs_Receive(CommDriver *dev, FDCAN_ClassicFrameType *frame)
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

void CANABS_CheckIsrPollPeriod(uint64_t timestamp, uint64_t timerPeriod)
{
    bool PeriodReachedCan1;
    bool PeriodReachedCan2;
    bool AnyFrameAvailableCan1;
    bool AnyFrameAvailableCan2;
    uint64_t LastIsrTimestamp = 0;
    FDCAN_HandleTypeDef *hfdcantmp;
    comm_status_t res;
    
    res = FDCAN_GetMostRecentTimestamp(&Fdcan1Driver, &LastIsrTimestamp);
    
    if (COMM_SUCCESS == res)
    {
        PeriodReachedCan1 = (timestamp - LastIsrTimestamp) >= timerPeriod;
    }
    else 
    {
        PeriodReachedCan1 = false;
    }

    res = FDCAN_GetMostRecentTimestamp(&Fdcan2Driver, &LastIsrTimestamp);

    if (COMM_SUCCESS == res)
    {
        PeriodReachedCan2 = (timestamp - LastIsrTimestamp) >= timerPeriod;
    }
    else 
    {
        PeriodReachedCan2 = false;
    }

    if (!PeriodReachedCan1 && ! PeriodReachedCan2)
    {
        return;
    }

    AnyFrameAvailableCan1 = false;
    AnyFrameAvailableCan2 = false;

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan1Driver, &hfdcantmp))
    {
        AnyFrameAvailableCan1 = HAL_FDCAN_GetRxFifoFillLevel(hfdcantmp, FDCAN_RX_FIFO0) > 0;
    }
    else 
    {
        AnyFrameAvailableCan1 = false;
        FDCAN_ErrorHandler();
    }

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan2Driver, &hfdcantmp))
    {
        AnyFrameAvailableCan2 = HAL_FDCAN_GetRxFifoFillLevel(hfdcantmp, FDCAN_RX_FIFO0) > 0;
    }
    else 
    {
        AnyFrameAvailableCan2 = false;
        FDCAN_ErrorHandler();
    }

#if CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ
    if (AnyFrameAvailableCan1 || AnyFrameAvailableCan2)
    {
        NVIC_SetPendingIRQ(FDCAN_1_IRQn);
    }
#else
    if (AnyFrameAvailableCan1)
    {
        NVIC_SetPendingIRQ(FDCAN_1_IRQn);
    }

    if (AnyFrameAvailableCan2)
    {
        NVIC_SetPendingIRQ(FDCAN_2_IRQn);
    }
#endif
}

void NotifyConsumerTask(void)
{
    CanAbs_RxNotificationCallback();
}

/**
 * @brief Reconstructs a full 64-bit timestamp from a hardware capture timestamp.
 *
 * Uses the current system timestamp to determine which timer epoch the captured
 * timestamp belongs to, then applies a monotonic correction based on the last
 * reconstructed timestamp when it is recent enough to be trusted.
 *
 * The epoch selection compares the captured timestamp with the current timer
 * position:
 * - Large positive difference (> Period/2): frame from previous epoch (wrapped).
 * - Large negative difference (< -Period/2): frame from next epoch (rare edge case).
 * - Otherwise: frame from current epoch (normal case).
 *
 * @param[in] hardware_timestamp  Timestamp captured by CAN peripheral at frame arrival,
 *                                already converted to microseconds.
 * @param[in] global_timestamp    Current system timestamp read in ISR (microseconds).
 * @param[inout] last_timestamp   Last absolute reconstructed 64-bit timestamp.
 *
 * @return Full 64-bit absolute timestamp of frame arrival.
 *
 * @note ISR latency should be < Period/2 for correct epoch detection.
 * @note Time between ISR calls should be < Period (as enforced by CANABS_CheckIsrPollPeriod).
 * @note Monotonic correction is only applied when last_timestamp is recent.
 * @note Returns hardware_timestamp unchanged if Period is 0.
 */
static uint64_t ReconstructFullTimestamp(
    uint64_t hardware_timestamp, 
    uint64_t global_timestamp,
    uint64_t *last_timestamp
)
{
    uint64_t Period;
    uint64_t current_timer_value;
    uint64_t epoch_count;
    int64_t time_diff;
    uint64_t Delta;
    uint64_t ReconstructedTimestamp;

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

    // Reconstruct full timestamp
    ReconstructedTimestamp = ((uint64_t)epoch_count * Period) + hardware_timestamp;

    if (*last_timestamp != 0 &&
        *last_timestamp <= global_timestamp &&
        (global_timestamp - *last_timestamp) <= (2 * Period)) 
    {                
        if (*last_timestamp > ReconstructedTimestamp + (Period / 2)) {
            Delta = *last_timestamp - (ReconstructedTimestamp + (Period / 2));
            ReconstructedTimestamp += ((Delta + Period - 1) / Period) * Period;
        }
    }

    *last_timestamp = ReconstructedTimestamp;
    
    return ReconstructedTimestamp;
}

/**
  * @brief  Rx FIFO 0 callback.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  RxFifo0ITs: indicates which Rx FIFO 0 interrupts are signalled.
  *                     This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
  * @retval None
  */
static uint32_t CanAbs_ReadAllAvailableFrames(CommDriver *driver,
                                              FDCAN_HandleTypeDef *hfdcan,
                                              uint8_t channel,
                                              uint32_t RxFifo0ITs,
                                              volatile uint64_t *rxOverflowDropCount)
{
    FDCAN_ClassicFrameType NewFrame;
    uint32_t frames_processed = 0;
    uint32_t fill_level = 0;
    uint64_t GlobalTimestamp;
    uint64_t HardwareTimestamp;
    RingBufferErrorType res;

    if (hfdcan->Instance->RXF0S & FDCAN_RXF0S_RF0L)
    {
        hfdcan->Instance->IR = FDCAN_IR_RF0L;
        if (rxOverflowDropCount != NULL)
        {
            (*rxOverflowDropCount)++;
        }
    }

    while (0 < (fill_level = HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0)))
    {
        if (driver->interface->read(driver, (void*)&NewFrame, 8u, RxFifo0ITs) == COMM_SUCCESS)
        {
            NewFrame.channel = channel;
            GlobalTimestamp = FDCAN_GetMostRecentInterruptTimestamp(driver);
            HardwareTimestamp = CANABS_ConvertCountToTimestampHook(NewFrame.timestamp);
            
            switch (channel)
            {
                case 1U:
                    NewFrame.timestamp = ReconstructFullTimestamp(
                        HardwareTimestamp, 
                        GlobalTimestamp,
                        &LastHardwareTimestampCan1
                    );
                    break;
                case 2U:
                    NewFrame.timestamp = ReconstructFullTimestamp(
                        HardwareTimestamp, 
                        GlobalTimestamp,
                        &LastHardwareTimestampCan2
                    );
                    break;
                default:
                    break;
            }

            res = ring_buffer_put((RingBuffer *)driver->RxFrameBuffer, (void*)&NewFrame);
            if (RB_E_OK != res)
            {
                CanAbs_FrameDropCount++;
                CanAbs_ErrorHandler();
            }
            frames_processed++;

            CanAbs_FrameCount++;
        }
        else
        {
            CanAbs_FrameDropCount++;
            CanAbs_ErrorHandler();
            break;
        }
    }

    return frames_processed;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    uint32_t frames_processed = 0;
    FDCAN_HandleTypeDef *hfdcantmp;

    CanAbs_InstrumentationIsrStartHook();

#if !CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ
    hfdcantmp = hfdcan;
    
    if (FDCAN_1 == hfdcantmp->Instance)
#else
    (void) fdcan_get_can(&Fdcan1Driver, &hfdcantmp);
#endif
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan1Driver, 
            hfdcantmp, 
            1u, 
            RxFifo0ITs, 
            &CanAbs_CAN1_Rx_FrameDropCount);
    }
#if !CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ
    else if ( FDCAN_2 == hfdcantmp->Instance)
#else
    (void) fdcan_get_can(&Fdcan2Driver, &hfdcantmp);
#endif
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan2Driver, 
            hfdcantmp, 
            2u, 
            RxFifo0ITs, 
            &CanAbs_CAN2_Rx_FrameDropCount);
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

comm_status_t CanAbs_Receive_Can1(FDCAN_ClassicFrameType *frame)
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

comm_status_t CanAbs_Receive_Can2(FDCAN_ClassicFrameType *frame)
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

void CanAbs_Drain(void)
{
    FDCAN_HandleTypeDef *hfdcantmp;
    uint32_t frames_processed = 0;

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan1Driver, &hfdcantmp))
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(&Fdcan1Driver, hfdcantmp, 1u, 0u, &CanAbs_CAN1_Rx_FrameDropCount);
    }

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan2Driver, &hfdcantmp))
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(&Fdcan2Driver, hfdcantmp, 2u, 0u, &CanAbs_CAN2_Rx_FrameDropCount);
    }

    if (frames_processed > 0)
    {
        NotifyConsumerTask();
    }
}

__attribute__((weak))
void  CanAbs_ErrorHandler(void)
{
    ;
}
