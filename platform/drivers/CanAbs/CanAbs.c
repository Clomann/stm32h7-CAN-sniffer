#include "CanAbs.h"
#include "CommTypes.h"
#include "buffers.h"
#include "fdcan.h"

#include <stdint.h>
#include "RuntimeChecks.h"
#include "stm32h745xx.h"
#include "stm32h7xx_hal_fdcan.h"
#include "stm32h7xx_hal_cortex.h"

#ifndef CANABS_RX_ISR_MAX_FRAMES_PER_CHANNEL
#define CANABS_RX_ISR_MAX_FRAMES_PER_CHANNEL FDCAN_IRQ_RX_WATERMARK
#endif

/**
 * @brief Hook called at CAN ISR entry for measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanAbs_InstrumentationIsrStartHook(void)
{
}

/**
 * @brief Hook called at CAN ISR exit for measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanAbs_InstrumentationIsrEndHook(void)
{
}

/* CAN 1 */
static uint64_t LastHardwareTimestampCan1 = 0;

static CommDriver Fdcan1Driver;
static CommDriverConfigType Fdcan1Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_1
};
static FDCAN_ClassicFrameType Fdcan1RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0
};
static FDCAN_ClassicFrameType Fdcan1TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0
};
static RingBuffer Fdcan1RxRingBuffer                                       = {
                                          .startAddress = &Fdcan1RxFrameBuffer[0],
                                          .head         = 0,
                                          .tail         = 0,
                                          .bufferLength =
        sizeof(Fdcan1RxFrameBuffer) / sizeof(Fdcan1RxFrameBuffer[0]),
                                          .elementSize = sizeof(Fdcan1RxFrameBuffer[0]),
                                          .stride      = sizeof(Fdcan1RxFrameBuffer[0]),
                                          .isFull      = false
};

static RingBuffer Fdcan1TxRingBuffer = {
    .startAddress = &Fdcan1TxFrameBuffer[0],
    .head         = 0,
    .tail         = 0,
    .bufferLength =
        sizeof(Fdcan1TxFrameBuffer) / sizeof(Fdcan1TxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan1TxFrameBuffer[0]),
    .stride      = sizeof(Fdcan1TxFrameBuffer[0]),
    .isFull      = false
};

/* CAN 2 */
static uint64_t LastHardwareTimestampCan2 = 0;

static CommDriver Fdcan2Driver;
static CommDriverConfigType Fdcan2Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_2
};
static FDCAN_ClassicFrameType Fdcan2RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0
};
static FDCAN_ClassicFrameType Fdcan2TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0
};
static RingBuffer Fdcan2RxRingBuffer                                       = {
                                          .startAddress = &Fdcan2RxFrameBuffer[0],
                                          .head         = 0,
                                          .tail         = 0,
                                          .bufferLength =
        sizeof(Fdcan2RxFrameBuffer) / sizeof(Fdcan2RxFrameBuffer[0]),
                                          .elementSize = sizeof(Fdcan2RxFrameBuffer[0]),
                                          .stride      = sizeof(Fdcan2RxFrameBuffer[0]),
                                          .isFull      = false
};

static RingBuffer Fdcan2TxRingBuffer = {
    .startAddress = &Fdcan2TxFrameBuffer[0],
    .head         = 0,
    .tail         = 0,
    .bufferLength =
        sizeof(Fdcan2TxFrameBuffer) / sizeof(Fdcan2TxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan2TxFrameBuffer[0]),
    .stride      = sizeof(Fdcan2TxFrameBuffer[0]),
    .isFull      = false
};

static uint32_t CanAbs_Can1_HwHighWatermark = 0U;
static uint32_t CanAbs_Can2_HwHighWatermark = 0U;
static uint32_t CanAbs_Can1_RxHighWater     = 0U;
static uint32_t CanAbs_Can2_RxHighWater     = 0U;

typedef struct
{
    volatile uint32_t protocol_status_latched;
    volatile uint32_t error_counter_latched;
    volatile uint32_t rx_fifo0_status_latched;
    volatile uint32_t interrupt_flags_latched;
    volatile uint32_t active_error_event_ir_flags;
    volatile uint8_t rx_fifo0_lost_active;
    volatile uint64_t rx_fifo0_lost_events;
    volatile uint64_t protocol_error_events;
    volatile uint64_t error_warning_events;
    volatile uint64_t error_passive_events;
    volatile uint64_t bus_off_events;
} CanAbsFdcanHealthStateType;

static CanAbsFdcanHealthStateType CanAbs_Can1_FdcanHealth = {0U};
static CanAbsFdcanHealthStateType CanAbs_Can2_FdcanHealth = {0U};

static uint32_t Can1_SequenceIndex = 0U;
static uint32_t Can2_SequenceIndex = 0U;

/* Private functions */

#define CANABS_FDCAN_PROTOCOL_ERROR_IR_MASK                                    \
    (FDCAN_IR_ELO | FDCAN_IR_PEA | FDCAN_IR_PED | FDCAN_IR_ARA)

#define CANABS_FDCAN_ERROR_STATE_IR_MASK                                       \
    (FDCAN_IR_EP | FDCAN_IR_EW | FDCAN_IR_BO)

#define CANABS_FDCAN_ERROR_EVENT_IR_MASK                                       \
    (CANABS_FDCAN_PROTOCOL_ERROR_IR_MASK | CANABS_FDCAN_ERROR_STATE_IR_MASK)

#define CANABS_FDCAN_LAST_ERROR_CODE_NO_ERROR  0U
#define CANABS_FDCAN_LAST_ERROR_CODE_NO_CHANGE 7U

/**
 * Little helper function to make ioctl function call a bit prettier.
 */
static inline int can_ioctl(struct CommDriver *dev, int cmd, void *arg)
{
    return dev->interface->ioctl(dev, cmd, arg);
}

static void CanAbs_UpdateRxHighWater(uint8_t channel, const RingBuffer *rb)
{
    uint32_t used = ring_buffer_count(rb);

    if (channel == 1U)
    {
        if (used > CanAbs_Can1_RxHighWater)
        {
            CanAbs_Can1_RxHighWater = used;
        }
    }
    else if (channel == 2U)
    {
        if (used > CanAbs_Can2_RxHighWater)
        {
            CanAbs_Can2_RxHighWater = used;
        }
    }
}

static CanAbsFdcanHealthStateType *CanAbs_GetFdcanHealthState(uint8_t channel)
{
    if (channel == 1U)
    {
        return &CanAbs_Can1_FdcanHealth;
    }
    else if (channel == 2U)
    {
        return &CanAbs_Can2_FdcanHealth;
    }

    return NULL;
}

static uint8_t CanAbs_IsLastErrorCodeActive(uint32_t code)
{
    return ((code != CANABS_FDCAN_LAST_ERROR_CODE_NO_ERROR)
            && (code != CANABS_FDCAN_LAST_ERROR_CODE_NO_CHANGE))
               ? 1U
               : 0U;
}

static uint8_t CanAbs_PsrHasInterestingError(uint32_t psr)
{
    uint32_t lec  = (psr & FDCAN_PSR_LEC) >> FDCAN_PSR_LEC_Pos;
    uint32_t dlec = (psr & FDCAN_PSR_DLEC) >> FDCAN_PSR_DLEC_Pos;

    if (0U
        != (psr & (FDCAN_PSR_EP | FDCAN_PSR_EW | FDCAN_PSR_BO | FDCAN_PSR_PXE)))
    {
        return 1U;
    }

    if ((0U != CanAbs_IsLastErrorCodeActive(lec))
        || (0U != CanAbs_IsLastErrorCodeActive(dlec)))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t CanAbs_EcrHasInterestingError(uint32_t ecr)
{
    return (0U
            != (ecr
                & (FDCAN_ECR_TEC | FDCAN_ECR_REC | FDCAN_ECR_RP | FDCAN_ECR_CEL)
            ))
               ? 1U
               : 0U;
}

static uint8_t
CanAbs_HasFdcanHealthLatch(const CanAbsFdcanHealthStateType *state)
{
    if (state == NULL)
    {
        return 0U;
    }

    return (0U
            != (state->protocol_status_latched | state->error_counter_latched
                | state->rx_fifo0_status_latched
                | state->interrupt_flags_latched))
               ? 1U
               : 0U;
}

static void CanAbs_ResetFdcanHealthState(CanAbsFdcanHealthStateType *state)
{
    if (state == NULL)
    {
        return;
    }

    state->protocol_status_latched     = 0U;
    state->error_counter_latched       = 0U;
    state->rx_fifo0_status_latched     = 0U;
    state->interrupt_flags_latched     = 0U;
    state->active_error_event_ir_flags = 0U;
    state->rx_fifo0_lost_active        = 0U;
    state->rx_fifo0_lost_events        = 0U;
    state->protocol_error_events       = 0U;
    state->error_warning_events        = 0U;
    state->error_passive_events        = 0U;
    state->bus_off_events              = 0U;
}

static void CanAbs_PrimeFdcanHealthState(
    CommDriver *driver,
    CanAbsFdcanHealthStateType *state
)
{
    FDCAN_HandleTypeDef *hfdcan;
    uint32_t rxf0s;
    uint32_t ir;

    if ((driver == NULL) || (state == NULL))
    {
        return;
    }

    if (COMM_SUCCESS != fdcan_get_can(driver, &hfdcan))
    {
        return;
    }

    if ((hfdcan == NULL) || (hfdcan->Instance == NULL))
    {
        return;
    }

    rxf0s = hfdcan->Instance->RXF0S;
    ir    = hfdcan->Instance->IR;

    state->rx_fifo0_lost_active =
        ((0U != (rxf0s & FDCAN_RXF0S_RF0L)) || (0U != (ir & FDCAN_IR_RF0L)))
            ? 1U
            : 0U;
    state->active_error_event_ir_flags = ir & CANABS_FDCAN_ERROR_EVENT_IR_MASK;
}

static void
CanAbs_CaptureFdcanHealth(uint8_t channel, FDCAN_HandleTypeDef *hfdcan)
{
    CanAbsFdcanHealthStateType *state;
    uint32_t psr;
    uint32_t ecr;
    uint32_t rxf0s;
    uint32_t ir;
    uint32_t active_error_event_ir_flags;
    uint32_t new_error_event_ir_flags;
    uint8_t rx_fifo0_lost_active;
    uint8_t latch_snapshot = 0U;

    if ((hfdcan == NULL) || (hfdcan->Instance == NULL))
    {
        return;
    }

    state = CanAbs_GetFdcanHealthState(channel);
    if (state == NULL)
    {
        return;
    }

    psr   = hfdcan->Instance->PSR;
    ecr   = hfdcan->Instance->ECR;
    rxf0s = hfdcan->Instance->RXF0S;
    ir    = hfdcan->Instance->IR;

    rx_fifo0_lost_active =
        ((0U != (rxf0s & FDCAN_RXF0S_RF0L)) || (0U != (ir & FDCAN_IR_RF0L)))
            ? 1U
            : 0U;
    if ((rx_fifo0_lost_active != 0U) && (state->rx_fifo0_lost_active == 0U))
    {
        state->rx_fifo0_lost_events++;
        latch_snapshot = 1U;
    }
    state->rx_fifo0_lost_active = rx_fifo0_lost_active;

    active_error_event_ir_flags = ir & CANABS_FDCAN_ERROR_EVENT_IR_MASK;
    new_error_event_ir_flags =
        active_error_event_ir_flags & ~state->active_error_event_ir_flags;
    state->active_error_event_ir_flags = active_error_event_ir_flags;

    if (0U != (new_error_event_ir_flags & CANABS_FDCAN_PROTOCOL_ERROR_IR_MASK))
    {
        state->protocol_error_events++;
        latch_snapshot = 1U;
    }

    if (0U != (new_error_event_ir_flags & FDCAN_IR_EW))
    {
        state->error_warning_events++;
        latch_snapshot = 1U;
    }

    if (0U != (new_error_event_ir_flags & FDCAN_IR_EP))
    {
        state->error_passive_events++;
        latch_snapshot = 1U;
    }

    if (0U != (new_error_event_ir_flags & FDCAN_IR_BO))
    {
        state->bus_off_events++;
        latch_snapshot = 1U;
    }

    if ((latch_snapshot == 0U) && (0U == CanAbs_HasFdcanHealthLatch(state))
        && ((0U != CanAbs_PsrHasInterestingError(psr))
            || (0U != CanAbs_EcrHasInterestingError(ecr))))
    {
        latch_snapshot = 1U;
    }

    if (latch_snapshot != 0U)
    {
        state->protocol_status_latched = psr;
        state->error_counter_latched   = ecr;
        state->rx_fifo0_status_latched = rxf0s;
        state->interrupt_flags_latched = ir;
    }
}

static uint8_t CanAbs_GetFdcanHealth(
    CommDriver *driver,
    CanAbsFdcanHealthStateType *state,
    CanAbsFdcanHealthStatsType *stats
)
{
    FDCAN_HandleTypeDef *hfdcan;

    if ((driver == NULL) || (state == NULL) || (stats == NULL))
    {
        return 1U;
    }

    if (COMM_SUCCESS != fdcan_get_can(driver, &hfdcan))
    {
        return 1U;
    }

    if ((hfdcan == NULL) || (hfdcan->Instance == NULL))
    {
        return 1U;
    }

    stats->protocol_status         = hfdcan->Instance->PSR;
    stats->error_counter           = hfdcan->Instance->ECR;
    stats->rx_fifo0_status         = hfdcan->Instance->RXF0S;
    stats->interrupt_flags         = hfdcan->Instance->IR;
    stats->protocol_status_latched = state->protocol_status_latched;
    stats->error_counter_latched   = state->error_counter_latched;
    stats->rx_fifo0_status_latched = state->rx_fifo0_status_latched;
    stats->interrupt_flags_latched = state->interrupt_flags_latched;
    stats->rx_fifo0_lost_events    = state->rx_fifo0_lost_events;
    stats->protocol_error_events   = state->protocol_error_events;
    stats->error_warning_events    = state->error_warning_events;
    stats->error_passive_events    = state->error_passive_events;
    stats->bus_off_events          = state->bus_off_events;

    return 0U;
}

#if CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ

#define CANABS_FDCAN_RX_FIFO0_MASK                                             \
    (FDCAN_IR_RF0L | FDCAN_IR_RF0F | FDCAN_IR_RF0W | FDCAN_IR_RF0N)

static void CanAbs_ClearRxFifo0IrqIfEmpty(CommDriver *driver)
{
    FDCAN_HandleTypeDef *hfdcan;
    uint32_t pending;

    if (COMM_SUCCESS != fdcan_get_can(driver, &hfdcan))
    {
        return;
    }

    if (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) != 0U)
    {
        return;
    }

    /* Avoid back-to-back IRQs from stale RX FIFO0 flags after draining. */
    pending = hfdcan->Instance->IR & CANABS_FDCAN_RX_FIFO0_MASK;
    pending &= hfdcan->Instance->IE;

    if (pending != 0U)
    {
        __HAL_FDCAN_CLEAR_FLAG(hfdcan, pending);
    }
}
#endif

int CanAbs_Init(
    CommDriver *dev,
    CommDriverConfigType *cfg,
    uint8_t *tx,
    uint8_t *rx
)
{
    unsigned int res = COMM_SUCCESS;

    dev->protocol = DRIVER_FDCAN;

    (void)CommManager_Init(
        dev,
        (const void *)cfg,
        sizeof(CommDriverConfigType),
        tx,
        rx
    );

    if (dev->interface->init(dev) != COMM_SUCCESS)
    {
        res = 1;
    }

    return res;
}

int CanAbs_Receive(CommDriver *dev, FDCAN_ClassicFrameType *frame)
{
    return ring_buffer_pop((RingBuffer *)dev->RxFrameBuffer, (void *)frame);
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
    uint32_t length
)
{

    {
        pMsg->can_id       = id;
        pMsg->isExtendedId = 0U;
        pMsg->frame_type   = 0U;
        pMsg->msgMarker    = 0U;

        pMsg->msgBase.dir           = DRIVER_MSGDIRECTION_TX;
        pMsg->msgBase.isMmultiframe = 0;
        pMsg->msgBase.length        = length;
        pMsg->msgBase.payload       = pData;
        pMsg->msgBase.protocol      = DRIVER_FDCAN;
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

    if (!PeriodReachedCan1 && !PeriodReachedCan2)
    {
        return;
    }

    AnyFrameAvailableCan1 = false;
    AnyFrameAvailableCan2 = false;

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan1Driver, &hfdcantmp))
    {
        AnyFrameAvailableCan1 =
            HAL_FDCAN_GetRxFifoFillLevel(hfdcantmp, FDCAN_RX_FIFO0) > 0;
    }
    else
    {
        AnyFrameAvailableCan1 = false;
        FDCAN_ErrorHandler();
    }

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan2Driver, &hfdcantmp))
    {
        AnyFrameAvailableCan2 =
            HAL_FDCAN_GetRxFifoFillLevel(hfdcantmp, FDCAN_RX_FIFO0) > 0;
    }
    else
    {
        AnyFrameAvailableCan2 = false;
        FDCAN_ErrorHandler();
    }

    bool bIsPendingCan1 = false;
    bool bIsPendingCan2 = false;

    bIsPendingCan1 = NVIC_GetPendingIRQ(FDCAN_1_IRQn) > 0;
    bIsPendingCan2 = NVIC_GetPendingIRQ(FDCAN_2_IRQn) > 0;

#if CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ

    if (bIsPendingCan1 || bIsPendingCan2)
    {
        // fall through
    }
    else if (AnyFrameAvailableCan1 || AnyFrameAvailableCan2)
    {
        NVIC_SetPendingIRQ(FDCAN_1_IRQn);
    }
#else
    if (AnyFrameAvailableCan1 && !bIsPendingCan1)
    {
        NVIC_SetPendingIRQ(FDCAN_1_IRQn);
    }

    if (AnyFrameAvailableCan2 && !bIsPendingCan2)
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
    epoch_count         = global_timestamp / Period;

    time_diff = (int64_t)hardware_timestamp - (int64_t)current_timer_value;

    // Determine if frame arrived in previous epoch
    if (time_diff > (int64_t)(Period / 2))
    {
        epoch_count--;
    }
    // Handle case where frame might be from next epoch
    else if (time_diff < -(int64_t)(Period / 2))
    {
        epoch_count++;
    }

    // Reconstruct full timestamp
    ReconstructedTimestamp =
        ((uint64_t)epoch_count * Period) + hardware_timestamp;

    if (*last_timestamp != 0 && *last_timestamp <= global_timestamp
        && (global_timestamp - *last_timestamp) <= (2 * Period))
    {
        if (*last_timestamp > ReconstructedTimestamp + (Period / 2))
        {
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
static uint32_t CanAbs_ReadAllAvailableFrames(
    CommDriver *driver,
    FDCAN_HandleTypeDef *hfdcan,
    uint8_t channel,
    uint32_t RxFifo0ITs,
    volatile uint64_t *rxOverflowDropCount
)
{
    FDCAN_ClassicFrameType NewFrame;
    uint32_t frames_processed = 0;
    uint32_t fill_level       = 0;
    uint64_t GlobalTimestamp;
    uint64_t HardwareTimestamp;
    RingBufferErrorType res;

    CanAbs_CaptureFdcanHealth(channel, hfdcan);

    if (((RxFifo0ITs & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != 0U)
        || ((hfdcan->Instance->RXF0S & FDCAN_RXF0S_RF0L) != 0U))
    {
        __HAL_FDCAN_CLEAR_FLAG(hfdcan, FDCAN_FLAG_RX_FIFO0_MESSAGE_LOST);
        if (rxOverflowDropCount != NULL)
        {
            (*rxOverflowDropCount)++;
        }
    }

    while ((frames_processed < CANABS_RX_ISR_MAX_FRAMES_PER_CHANNEL)
           && (0
               < (fill_level =
                      HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0))))
    {
        switch (channel)
        {
        case 1U:
            if (fill_level > CanAbs_Can1_HwHighWatermark)
            {
                CanAbs_Can1_HwHighWatermark = fill_level;
            }
            break;
        case 2U:
            if (fill_level > CanAbs_Can2_HwHighWatermark)
            {
                CanAbs_Can2_HwHighWatermark = fill_level;
            }
            break;
        default:
            break;
        }

        if (driver->interface->read(driver, (void *)&NewFrame, 8u, RxFifo0ITs)
            == COMM_SUCCESS)
        {
            NewFrame.channel = channel;
            GlobalTimestamp  = FDCAN_GetMostRecentInterruptTimestamp(driver);
            HardwareTimestamp =
                CANABS_ConvertCountToTimestampHook(NewFrame.timestamp);

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
                NewFrame.timestamp   = 0U;
                NewFrame.rx_sequence = 0U;
                break;
            }

            switch (channel)
            {
            case 1U:
                Can1_SequenceIndex += 1U;
                NewFrame.rx_sequence = Can1_SequenceIndex;
                break;
            case 2U:
                Can2_SequenceIndex += 1U;
                NewFrame.rx_sequence = Can2_SequenceIndex;
                break;
            default:
                NewFrame.timestamp   = 0U;
                NewFrame.rx_sequence = 0U;
                break;
            }

            res = ring_buffer_put(
                (RingBuffer *)driver->RxFrameBuffer,
                (void *)&NewFrame
            );
            if (RB_E_OK != res)
            {
                CanAbs_FrameDropCount++;
                CanAbs_ErrorHandler();
            }
            else
            {
                CanAbs_UpdateRxHighWater(
                    channel,
                    (RingBuffer *)driver->RxFrameBuffer
                );
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
    (void)fdcan_get_can(&Fdcan1Driver, &hfdcantmp);
#endif
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan1Driver,
            hfdcantmp,
            1u,
            RxFifo0ITs,
            &CanAbs_CAN1_Rx_FrameDropCount
        );
    }
#if !CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ
    else if (FDCAN_2 == hfdcantmp->Instance)
#else
    (void)fdcan_get_can(&Fdcan2Driver, &hfdcantmp);
#endif
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan2Driver,
            hfdcantmp,
            2u,
            RxFifo0ITs,
            &CanAbs_CAN2_Rx_FrameDropCount
        );
    }

#if CANABS_CONSUME_ALL_FRAMES_ON_ANY_IRQ
    CanAbs_ClearRxFifo0IrqIfEmpty(&Fdcan1Driver);
    CanAbs_ClearRxFifo0IrqIfEmpty(&Fdcan2Driver);
#endif

    if (frames_processed > 0)
    {
        NotifyConsumerTask();
    }

    CanAbs_InstrumentationIsrEndHook();
}

/* Public functions ================================================== */

/* CAN 1 */
comm_status_t CanAbs_Init_Can1(uint32_t baudrate)
{
    comm_status_t res;

    CanAbs_Can1_RxHighWater     = 0U;
    CanAbs_Can1_HwHighWatermark = 0U;
    CanAbs_ResetFdcanHealthState(&CanAbs_Can1_FdcanHealth);

    res = CanAbs_Init(
        &Fdcan1Driver,
        &Fdcan1Config,
        (uint8_t *)&Fdcan1TxRingBuffer,
        (uint8_t *)&Fdcan1RxRingBuffer
    );

    if (COMM_SUCCESS == res)
    {
        Fdcan1Driver.interface->ioctl(
            &Fdcan1Driver,
            CANABS_IOCTL_CMD_SET_BAUDRATE,
            (void *)&baudrate
        );
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

comm_status_t CanAbs_IsStateOff_Can1(bool *isOff)
{
    *isOff = Fdcan1Driver.state == DRIVER_STATE_OFF;
    return COMM_SUCCESS;
}

/* CAN 2 */

comm_status_t CanAbs_Init_Can2(uint32_t baudrate)
{
    comm_status_t res;

    CanAbs_Can2_RxHighWater     = 0U;
    CanAbs_Can2_HwHighWatermark = 0U;
    CanAbs_ResetFdcanHealthState(&CanAbs_Can2_FdcanHealth);

    res = CanAbs_Init(
        &Fdcan2Driver,
        &Fdcan2Config,
        (uint8_t *)&Fdcan2TxRingBuffer,
        (uint8_t *)&Fdcan2RxRingBuffer
    );

    if (COMM_SUCCESS == res)
    {
        Fdcan2Driver.interface->ioctl(
            &Fdcan2Driver,
            CANABS_IOCTL_CMD_SET_BAUDRATE,
            (void *)&baudrate
        );
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

comm_status_t CanAbs_IsStateOff_Can2(bool *isOff)
{
    *isOff = Fdcan2Driver.state == DRIVER_STATE_OFF;
    return COMM_SUCCESS;
}

uint8_t CanAbs_GetRxHighWater_Can1(uint32_t *frames)
{
    if (frames == NULL)
    {
        return 1U;
    }

    *frames = CanAbs_Can1_RxHighWater;
    return 0U;
}

uint8_t CanAbs_GetHwRxFifoHighWater_Can1(uint32_t *frames)
{
    if (frames == NULL)
    {
        return 1U;
    }

    *frames = CanAbs_Can1_HwHighWatermark;
    return 0U;
}

uint8_t CanAbs_GetStaticTxReplayStats_Can1(FdcanStaticTxReplayStatsType *stats)
{
#if CAN_STATIC_TX_REPLAY_ENABLE
    return (uint8_t)FDCAN_StaticTxReplayGetStats(&Fdcan1Driver, stats);
#else
    if (stats == NULL)
    {
        return 1U;
    }

    stats->requests            = 0U;
    stats->completed           = 0U;
    stats->irqs                = 0U;
    stats->last_completed_mask = 0U;
    stats->buffer_mask         = 0U;
    stats->next_id_offset      = 0U;
    stats->tx_pending          = 0U;
    stats->tx_occurred         = 0U;
    stats->tx_cancelled        = 0U;
    stats->protocol_status     = 0U;
    stats->error_counter       = 0U;
    stats->reload_errors       = 0U;
    stats->active              = 0U;
    return 0U;
#endif
}

uint8_t CanAbs_GetFdcanHealth_Can1(CanAbsFdcanHealthStatsType *stats)
{
    return CanAbs_GetFdcanHealth(
        &Fdcan1Driver,
        &CanAbs_Can1_FdcanHealth,
        stats
    );
}

uint8_t CanAbs_GetRxHighWater_Can2(uint32_t *frames)
{
    if (frames == NULL)
    {
        return 1U;
    }

    *frames = CanAbs_Can2_RxHighWater;
    return 0U;
}

uint8_t CanAbs_GetHwRxFifoHighWater_Can2(uint32_t *frames)
{
    if (frames == NULL)
    {
        return 1U;
    }

    *frames = CanAbs_Can2_HwHighWatermark;
    return 0U;
}

uint8_t CanAbs_GetStaticTxReplayStats_Can2(FdcanStaticTxReplayStatsType *stats)
{
#if CAN_STATIC_TX_REPLAY_ENABLE
    return (uint8_t)FDCAN_StaticTxReplayGetStats(&Fdcan2Driver, stats);
#else
    if (stats == NULL)
    {
        return 1U;
    }

    stats->requests            = 0U;
    stats->completed           = 0U;
    stats->irqs                = 0U;
    stats->last_completed_mask = 0U;
    stats->buffer_mask         = 0U;
    stats->next_id_offset      = 0U;
    stats->tx_pending          = 0U;
    stats->tx_occurred         = 0U;
    stats->tx_cancelled        = 0U;
    stats->protocol_status     = 0U;
    stats->error_counter       = 0U;
    stats->reload_errors       = 0U;
    stats->active              = 0U;
    return 0U;
#endif
}

uint8_t CanAbs_GetFdcanHealth_Can2(CanAbsFdcanHealthStatsType *stats)
{
    return CanAbs_GetFdcanHealth(
        &Fdcan2Driver,
        &CanAbs_Can2_FdcanHealth,
        stats
    );
}

uint32_t CanAbs_GetRxBufferCapacity(void)
{
    return SW_RX_FRAME_BUFFER_SIZE;
}

uint32_t CanAbs_GetHwRxFifoCapacity(void)
{
    return FDCAN_RAM_RX_ELEMENTS;
}

void CanAbs_ResetFdcanHealth(void)
{
    CanAbs_ResetFdcanHealthState(&CanAbs_Can1_FdcanHealth);
    CanAbs_ResetFdcanHealthState(&CanAbs_Can2_FdcanHealth);
    CanAbs_PrimeFdcanHealthState(&Fdcan1Driver, &CanAbs_Can1_FdcanHealth);
    CanAbs_PrimeFdcanHealthState(&Fdcan2Driver, &CanAbs_Can2_FdcanHealth);
}

void CanAbs_Drain(void)
{
    FDCAN_HandleTypeDef *hfdcantmp;
    uint32_t frames_processed = 0;

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan1Driver, &hfdcantmp))
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan1Driver,
            hfdcantmp,
            1u,
            0u,
            &CanAbs_CAN1_Rx_FrameDropCount
        );
    }

    if (COMM_SUCCESS == fdcan_get_can(&Fdcan2Driver, &hfdcantmp))
    {
        frames_processed += CanAbs_ReadAllAvailableFrames(
            &Fdcan2Driver,
            hfdcantmp,
            2u,
            0u,
            &CanAbs_CAN2_Rx_FrameDropCount
        );
    }

    if (frames_processed > 0)
    {
        NotifyConsumerTask();
    }
}

__attribute__((weak)) void CanAbs_ErrorHandler(void)
{
    ;
}
