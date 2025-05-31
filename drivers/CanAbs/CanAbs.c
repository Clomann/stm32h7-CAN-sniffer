#include "CanAbs.h"
#include "CanAbs_Cfg.h"
#include "buffers.h"
#include "fdcan.h"

static FDCAN_Message Msg1;
static FDCAN_Message Msg2;
static FDCAN_Message Msg3;
static FDCAN_Message Msg4;

static CommDriver Fdcan1Driver;
static CommDriverConfigType Fdcan1Config;
static FDCAN_ClassicFrame Fdcan1RxFrameBuffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
static FDCAN_ClassicFrame Fdcan1TxFrameBuffer[SW_TX_FRAME_BUFFER_SIZE] = {0};
static RingBuffer Fdcan1RxRingBuffer = {
    .startAddress = &Fdcan1RxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan1RxFrameBuffer) / sizeof(Fdcan1RxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan1RxFrameBuffer[0]),
    .isFull = false
};

static RingBuffer Fdcan1TxRingBuffer = {
    .startAddress = &Fdcan1TxFrameBuffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(Fdcan1TxFrameBuffer) / sizeof(Fdcan1TxFrameBuffer[0]),
    .elementSize = sizeof(Fdcan1TxFrameBuffer[0]),
    .isFull = false
};

/* Private functions */

/**
 * Little helper function to make ioctl function call a bit prettier.
 */
static inline int can_ioctl(struct CommDriver *dev, int cmd, void *arg) {
    return dev->interface->ioctl(dev, cmd, arg);
}

int CanAbs_Init(CommDriver *dev, CommDriverConfigType *cfg, RingBuffer *tx, RingBuffer *rx)
{
    uint8_t TxData[8];
    uint8_t TxData2[8];
    unsigned int res = COMM_SUCCESS;

    dev->protocol = DRIVER_FDCAN;
    cfg->config = DRIVER_CFG2;

    (void)CommManager_Init(dev, cfg, sizeof(CommDriverConfigType), tx, rx);

	if (dev->interface->init(dev) != COMM_SUCCESS)
	{
	  res = 1;
	}

    if (0 == res)
    {
        fdcan_create_message_1(&Msg1, &TxData[0], sizeof(TxData) / sizeof(*TxData));
        fdcan_create_message_2(&Msg2, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
        fdcan_create_message_3(&Msg3, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
        fdcan_create_message_4(&Msg4, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
    }

    return res;
}

int CanAbs_Receive(CommDriver *dev, FDCAN_ClassicFrame *frame)
{
    return ring_buffer_pop(dev->RxFrameBuffer, (void*)frame);
}

int CanAbs_Send(CommDriver *dev)
{
    int res = 0;

    if (dev->interface->send(dev, &Msg2) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 1;
    }

    if (dev->interface->send(dev, &Msg3) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 2;
    }

    if (dev->interface->send(dev, &Msg4) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 3;
    }

    if (0 != res)
    {
        FDCAN_ErrorHandler();
    }

    return res;
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

comm_status_t fdcan_create_message_1(FDCAN_Message *pMsg, uint8_t *pData, uint32_t length)
{

	{
		pMsg->can_id = 0x321U;
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

comm_status_t fdcan_create_message_2(FDCAN_Message *pMsg, uint8_t *pData, uint32_t length)
{

	{
		pMsg->can_id = 0x322U;
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

comm_status_t fdcan_create_message_3(FDCAN_Message *pMsg, uint8_t *pData, uint32_t length)
{

	{
		pMsg->can_id = 0x323U;
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

comm_status_t fdcan_create_message_4(FDCAN_Message *pMsg, uint8_t *pData, uint32_t length)
{

	{
		pMsg->can_id = 0x324U;
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

    if (Fdcan1Driver.interface->read(&Fdcan1Driver, (void*)&NewFrame, 8u, RxFifo0ITs) == COMM_SUCCESS)
    {
        FDCAN_GetMostRecentInterruptTimestamp(&Fdcan1Driver, &NewFrame.timestamp);
        ring_buffer_put(Fdcan1Driver.RxFrameBuffer, (void*)&NewFrame);
    }
}

/* Public functions */

comm_status_t CanAbs_Init_Can1()
{
    return CanAbs_Init(&Fdcan1Driver, &Fdcan1Config, &Fdcan1TxRingBuffer, &Fdcan1RxRingBuffer);
}

comm_status_t CanAbs_Send_Can1()
{
    return CanAbs_Send(&Fdcan1Driver);
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