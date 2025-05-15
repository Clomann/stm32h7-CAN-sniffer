#include "CanAbs.h"
#include "buffers.h"

#define SW_RX_FRAME_BUFFER_SIZE 128 /* software Rx frame buffer size in number of FDCAN_ClassicFrame elements */
#define SW_TX_FRAME_BUFFER_SIZE 128 /* software Rx frame buffer size in number of FDCAN_ClassicFrame elements */

uint32_t Msg1Id = 0x0;
FDCAN_Message Msg1;
FDCAN_Message Msg2;
FDCAN_Message Msg3;
FDCAN_Message Msg4;

CommDriver fdcan_driver;
FDCAN_ClassicFrame fdcan_rx_frame_buffer[SW_RX_FRAME_BUFFER_SIZE] = {0};
FDCAN_ClassicFrame fdcan_tx_frame_buffer[SW_TX_FRAME_BUFFER_SIZE] = {0};

RingBuffer fdcan_RxRingBuffer = {
    .startAddress = &fdcan_rx_frame_buffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(fdcan_rx_frame_buffer) / sizeof(fdcan_rx_frame_buffer[0]),
    .elementSize = sizeof(fdcan_rx_frame_buffer[0]),
    .isFull = false};

RingBuffer fdcan_TxRingBuffer = {
    .startAddress = &fdcan_tx_frame_buffer[0],
    .head = 0,
    .tail = 0,
    .bufferLength = sizeof(fdcan_tx_frame_buffer) / sizeof(fdcan_tx_frame_buffer[0]),
    .elementSize = sizeof(fdcan_tx_frame_buffer[0]),
    .isFull = false};

// FDCAN_HandleTypeDef hfdcan;
// FDCAN_RxHeaderTypeDef RxHeader;
// FDCAN_TxHeaderTypeDef TxHeader;
uint8_t RxData[8];
uint8_t TxData[8];
uint8_t TxData2[8];

int CanAbs_Init()
{
    unsigned int res;

    res = fdcan_setup();

    if (0 == res)
    {
        fdcan_create_message_1(&Msg1, &TxData[0], sizeof(TxData) / sizeof(*TxData));
        fdcan_create_message_2(&Msg2, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
        fdcan_create_message_3(&Msg3, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
        fdcan_create_message_4(&Msg4, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
    }

    return res;
}

int CanAbs_Receive(FDCAN_ClassicFrame *frame)
{
    return ring_buffer_pop(fdcan_driver.RxFrameBuffer, (void*)frame);
}

int CanAbs_Send()
{
    int res = 0;

    if (fdcan_driver.interface->send(&Msg2) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 1;
    }

    if (fdcan_driver.interface->send(&Msg3) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 2;
    }

    if (fdcan_driver.interface->send(&Msg4) != COMM_SUCCESS)
    {
        /* Transmission request Error */
        res = 3;
    }

    return res;
}


unsigned int fdcan_setup()
{
    unsigned int res = 0;

	(void)comm_manager_init(&fdcan_driver, DRIVER_FDCAN, DRIVER_CFG2, &fdcan_RxRingBuffer);

	if (fdcan_driver.interface->init() != COMM_SUCCESS)
	{
	  res = 1;
	}

    return res;
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
 
     if (fdcan_driver.interface->read((void*)&NewFrame, 8u, RxFifo0ITs) == COMM_SUCCESS)
     {
 
         ring_buffer_put(fdcan_driver.RxFrameBuffer, (void*)&NewFrame);
     }
 
 }