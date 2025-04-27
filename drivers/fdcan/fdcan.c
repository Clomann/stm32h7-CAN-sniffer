/*
 * fdcan.c
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

#include "fdcan.h"

typedef struct {
	CommInterface interface;
	FdcanConfigType config;            // Store the configuration for this instance
	void (*interrupt_callback)(void); // Interrupt callback for this instance
	int channel;                    // Channel for this instance
} FDCAN_Driver;

FDCAN_HandleTypeDef hfdcan;
FDCAN_RxHeaderTypeDef RxHeader;

FDCAN_Driver fdcan_drivers[MAX_INSTANCES];
FdcanConfigType fdcan_configs[DRIVER_CFGn];

/* Private function prototypes -----------------------------------------------*/
void FDCANx_IRQHandler(void);
comm_status_t get_fdcan_config(
		FDCAN_HandleTypeDef *,
		FDCAN_FilterTypeDef *,
		driver_cfg_t);

comm_status_t fdcan_init_tx_header(const void *, FDCAN_TxHeaderTypeDef *, uint32_t);

comm_status_t FDCAN_CreateDriver(CommDriver *pDriver, driver_cfg_t config, RingBuffer *pRxBuffer)
{
	comm_status_t RetVal;
	FdcanConfigType newConfig;
	FDCAN_FilterTypeDef pFilterConfig;

	RetVal = COMM_ERROR;

	pDriver->interface->init = FDCAN_Init;
	pDriver->interface->send = FDCAN_Send;
	pDriver->interface->read = FDCAN_Read;
	pDriver->configNbr = config;
	pDriver->protocol = DRIVER_FDCAN;
	pDriver->RxFrameBuffer = pRxBuffer;

	switch (config)
	{
		case DRIVER_CFG0:
		case DRIVER_CFG1:
		case DRIVER_CFG2:
			get_fdcan_config(&hfdcan, &pFilterConfig, config);
			memcpy(&fdcan_configs[config], &newConfig, sizeof(FdcanConfigType));
			pDriver->config = &(fdcan_configs[config]);
			pDriver->initialized = 1;
			RetVal = COMM_SUCCESS;
			break;
		default:
			RetVal = COMM_ERROR;
			return RetVal;
	}

	return RetVal;
}

FDCAN_Driver* create_driver(uint8_t instance) {
    if (instance < 1 || instance > MAX_INSTANCES) {
        return NULL;  // Error: Invalid channel
    }

    // Initialize the driver instance for this channel
    FDCAN_Driver *driver = &fdcan_drivers[instance - 1];
    driver->channel = instance;

    // Set up the function pointers in the interface
//    driver->interface.init = FDCAN_Init;
//    driver->interface.read = void*;
//    driver->interface.write = void*;
//    driver->interface.register_interrupt_callback = void*;

    return driver;
}

comm_status_t FDCAN_Init(FdcanDeviceType *dev, FdcanConfigType *cfg, uint8_t *rxBuf, uint32_t rxLen, uint8_t *txBuf, uint32_t txLen)
{
	FDCAN_FilterTypeDef sFilterConfig;
	comm_status_t RetVal;

    (void) dev;
    (void) cfg;
    (void) rxBuf;
    (void) rxLen;
    (void) txBuf;
    (void) txLen;


	RetVal = COMM_SUCCESS;

	// TODO make init function consistent with driver creation
	get_fdcan_config(&hfdcan, &sFilterConfig, DRIVER_CFG2);
	  if (HAL_FDCAN_Init(&hfdcan) != HAL_OK)
	  {
	    /* Initialization Error */
		  RetVal = COMM_ERROR;
	  }

	  if (HAL_FDCAN_ConfigTimestampCounter(&hfdcan, FDCAN_TIMESTAMP_PRESC_1) != HAL_OK)
	  {
		/* Initialization Error */
		  RetVal = COMM_ERROR;
	  }

	  if (HAL_FDCAN_EnableTimestampCounter(&hfdcan, FDCAN_TIMESTAMP_INTERNAL) != HAL_OK)
	  {
		/* Initialization Error */
		  RetVal = COMM_ERROR;
	  }

	  /* Configure Rx filter */
	  if (HAL_FDCAN_ConfigFilter(&hfdcan, &sFilterConfig) != HAL_OK)
	  {
	    /* Filter configuration Error */
		  RetVal = COMM_ERROR;
	  }

	  /* Start the FDCAN module */
	  if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
	  {
	    /* Start Error */
	    RetVal = COMM_ERROR;
	  }

	  if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
	  {
	    /* Notification Error */
		  RetVal = COMM_ERROR;
	  }

	  /* Prepare Tx Header */

	  return RetVal;
}

comm_status_t FDCAN_Send(const void *pMsg)
{
	comm_status_t RetVal;
	uint8_t * pData;
	FDCAN_Message *pMsgCpy;
	FDCAN_TxHeaderTypeDef TxHeader;

	RetVal = COMM_SUCCESS;
	pMsgCpy = (FDCAN_Message*)pMsg;

	pData = (uint8_t*)(pMsgCpy->msgBase.payload);

	(void)fdcan_init_tx_header(pMsgCpy, &TxHeader, pMsgCpy->msgBase.length);

	if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &TxHeader, pData) == HAL_OK)
	{
		RetVal = COMM_SUCCESS;
	}
	else
	{
		RetVal = COMM_ERROR;
	}

	return RetVal;
}

comm_status_t FDCAN_RxBuffer_Pop(void *pRxData, uint8_t length, uint32_t RxFifo0ITs)
{
    return 0;
}

comm_status_t FDCAN_Read(void *pFrame, uint8_t length, uint32_t RxFifo0ITs)
{
  comm_status_t RetVal;
  uint8_t Data[8];
  FDCAN_ClassicFrame *pNewFrame;

  RetVal = COMM_SUCCESS;
  pNewFrame = (FDCAN_ClassicFrame*)pFrame;

  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &RxHeader, Data) != HAL_OK)
    {
    	/* Reception Error */
    	RetVal = COMM_ERROR;
    }

    /* Display LEDx */
    if ((RxHeader.Identifier == 0x321) && (RxHeader.IdType == FDCAN_STANDARD_ID) && (RxHeader.DataLength == FDCAN_DLC_BYTES_2))
    {

    }

    switch (RxHeader.DataLength)
    {
		case FDCAN_DLC_BYTES_0:
			pNewFrame->dlc = 0;
			break;
		case FDCAN_DLC_BYTES_1:
			pNewFrame->dlc = 1;
			break;
		case FDCAN_DLC_BYTES_2:
			pNewFrame->dlc = 2;
			break;
		case FDCAN_DLC_BYTES_3:
			pNewFrame->dlc = 3;
			break;
		case FDCAN_DLC_BYTES_4:
			pNewFrame->dlc = 4;
			break;
		case FDCAN_DLC_BYTES_5:
			pNewFrame->dlc = 5;
			break;
		case FDCAN_DLC_BYTES_6:
			pNewFrame->dlc = 6;
			break;
		case FDCAN_DLC_BYTES_7:
			pNewFrame->dlc = 7;
			break;
		case FDCAN_DLC_BYTES_8:
			pNewFrame->dlc = 8;
			break;
		default:
			pNewFrame->dlc =  0;
    };


    pNewFrame->id = RxHeader.Identifier;
    pNewFrame->timestamp = RxHeader.RxTimestamp;

    memcpy(pNewFrame->data, &Data, pNewFrame->dlc);

    if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
      /* Notification Error */
    	RetVal = COMM_ERROR;
    }
  }

  return RetVal;
}

comm_status_t FDCAN_RegisterTxMessage(Message *pMsg)
{
	return COMM_ERROR;
}

comm_status_t FDCAN_RegisterRxMessage(Message *pMsg)
{
	return COMM_ERROR;
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

void FDCANx_IRQHandler(void)
{
  HAL_FDCAN_IRQHandler(&hfdcan);
}

/**
  * @brief  Initializes the FDCAN MSP.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @retval None
  */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* hfdcan)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  FDCANx_TX_GPIO_CLK_ENABLE();
  FDCANx_RX_GPIO_CLK_ENABLE();

  /* Select PLL1Q as source of FDCANx clock */
  RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
  RCC_PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

  /* Enable FDCANx clock */
  FDCANx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* FDCANx TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = FDCANx_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = FDCANx_TX_AF;
  HAL_GPIO_Init(FDCANx_TX_GPIO_PORT, &GPIO_InitStruct);

  /* FDCANx RX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = FDCANx_RX_PIN;
  GPIO_InitStruct.Alternate = FDCANx_RX_AF;
  HAL_GPIO_Init(FDCANx_RX_GPIO_PORT, &GPIO_InitStruct);

  /*##-3- Configure the NVIC #################################################*/
  /* NVIC for FDCANx */
  HAL_NVIC_SetPriority(FDCANx_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(FDCANx_IRQn);
}

/**
  * @brief  DeInitializes the FDCAN MSP.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @retval None
  */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* hfdcan)
{
  /*##-1- Reset peripherals ##################################################*/
  FDCANx_FORCE_RESET();
  FDCANx_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* Configure FDCANx Tx as alternate function  */
  HAL_GPIO_DeInit(FDCANx_TX_GPIO_PORT, FDCANx_TX_PIN);

  /* Configure FDCANx Rx as alternate function  */
  HAL_GPIO_DeInit(FDCANx_RX_GPIO_PORT, FDCANx_RX_PIN);

  /*##-3- Disable the NVIC for FDCANx ########################################*/
  HAL_NVIC_DisableIRQ(FDCANx_IRQn);
}

comm_status_t fdcan_init_tx_header(const void * pMsg, FDCAN_TxHeaderTypeDef *pTxHeader, uint32_t frameLength)
{
	comm_status_t RetVal;
	FDCAN_Message * pMsgCopy;

	RetVal = COMM_ERROR;
	pMsgCopy = (FDCAN_Message*)pMsg;

	pTxHeader->Identifier = pMsgCopy->can_id;

	if (true == pMsgCopy->isExtendedId)
	{
		pTxHeader->IdType = FDCAN_EXTENDED_ID;
	}
	else
	{
		pTxHeader->IdType = FDCAN_STANDARD_ID;
	}


	pTxHeader->TxFrameType = FDCAN_DATA_FRAME;

	switch (frameLength)
	{
		case 0:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_0;
			break;
		case 1:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_1;
			break;
		case 2:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_2;
			break;
		case 3:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_3;
			break;
		case 4:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_4;
			break;
		case 5:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_5;
			break;
		case 6:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_6;
			break;
		case 7:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_7;
			break;
		case 8:
			pTxHeader->DataLength = FDCAN_DLC_BYTES_8;
			break;
		default:
			break;
	}

	pTxHeader->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	pTxHeader->BitRateSwitch = FDCAN_BRS_OFF;
	pTxHeader->FDFormat = FDCAN_CLASSIC_CAN;
	pTxHeader->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	pTxHeader->MessageMarker = pMsgCopy->msgMarker;

	RetVal = COMM_SUCCESS;

	return RetVal;
}

comm_status_t get_fdcan_config(
		FDCAN_HandleTypeDef *pHfdcan,
		FDCAN_FilterTypeDef *pFilterConfig,
		driver_cfg_t config)
{
	comm_status_t RetVal;
	RetVal = COMM_ERROR;

	switch (config)
	{
		case DRIVER_CFG0:
			/*	Bit time configuration:
				fdcan_ker_ck               = 40 MHz
				Time_quantum (tq)          = 25 ns
				Synchronization_segment    = 1 tq
				Propagation_segment        = 23 tq
				Phase_segment_1            = 8 tq
				Phase_segment_2            = 8 tq
				Synchronization_Jump_width = 8 tq
				Bit_length                 = 40 tq = 1 �s
				Bit_rate                   = 1 MBit/s
			*/
			pHfdcan->Instance = FDCANx;
			pHfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
			pHfdcan->Init.Mode = FDCAN_MODE;
			pHfdcan->Init.AutoRetransmission = ENABLE;
			pHfdcan->Init.TransmitPause = DISABLE;
			pHfdcan->Init.ProtocolException = ENABLE;
			pHfdcan->Init.NominalPrescaler = 0x1; /* tq = NominalPrescaler x (1/fdcan_ker_ck) */
			pHfdcan->Init.NominalSyncJumpWidth = 0x8;
			pHfdcan->Init.NominalTimeSeg1 = 0x1F; /* NominalTimeSeg1 = Propagation_segment + Phase_segment_1 */
			pHfdcan->Init.NominalTimeSeg2 = 0x8;
			pHfdcan->Init.MessageRAMOffset = 0;
			pHfdcan->Init.StdFiltersNbr = 1;
			pHfdcan->Init.ExtFiltersNbr = 0;
			pHfdcan->Init.RxFifo0ElmtsNbr = 1;
			pHfdcan->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
			pHfdcan->Init.RxFifo1ElmtsNbr = 0;
			pHfdcan->Init.RxBuffersNbr = 0;
			pHfdcan->Init.TxEventsNbr = 0;
			pHfdcan->Init.TxBuffersNbr = 0;
			pHfdcan->Init.TxFifoQueueElmtsNbr = 1;
			pHfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
			pHfdcan->Init.TxElmtSize = FDCAN_DATA_BYTES_8;

			pFilterConfig->IdType = FDCAN_STANDARD_ID;
			pFilterConfig->FilterIndex = 0;
			pFilterConfig->FilterType = FDCAN_FILTER_RANGE;
			pFilterConfig->FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
			pFilterConfig->FilterID1 = 0x0;
			pFilterConfig->FilterID2 = 0x7FF;

			RetVal = COMM_SUCCESS;
			break;
		case DRIVER_CFG1:
			/*	Bit time configuration:
				fdcan_ker_ck               = 40 MHz
				Time_quantum (tq)          = 100 ns
				Synchronization_segment    = 1 tq
				Propagation_segment        =  tq
				Phase_segment_1            =  tq
				Phase_segment_2            =  tq
				Synchronization_Jump_width = 8 tq
				Bit_length                 = 10 tq
				Bit_rate                   = 1 MBit/s
			*/
			pHfdcan->Instance = FDCANx;
			pHfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
			pHfdcan->Init.Mode = FDCAN_MODE;
			pHfdcan->Init.AutoRetransmission = ENABLE;
			pHfdcan->Init.TransmitPause = DISABLE;
			pHfdcan->Init.ProtocolException = ENABLE;
			pHfdcan->Init.NominalPrescaler = 0x4; /* tq = NominalPrescaler x (1/fdcan_ker_ck) */
			pHfdcan->Init.NominalSyncJumpWidth = 0x8;
			pHfdcan->Init.NominalTimeSeg1 = 0x8; /* NominalTimeSeg1 = Propagation_segment + Phase_segment_1 */
			pHfdcan->Init.NominalTimeSeg2 = 0x1;
			pHfdcan->Init.MessageRAMOffset = 0;
			pHfdcan->Init.StdFiltersNbr = 1;
			pHfdcan->Init.ExtFiltersNbr = 0;
			pHfdcan->Init.RxFifo0ElmtsNbr = 1;
			pHfdcan->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
			pHfdcan->Init.RxFifo1ElmtsNbr = 0;
			pHfdcan->Init.RxBuffersNbr = 0;
			pHfdcan->Init.TxEventsNbr = 0;
			pHfdcan->Init.TxBuffersNbr = 0;
			pHfdcan->Init.TxFifoQueueElmtsNbr = 1;
			pHfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
			pHfdcan->Init.TxElmtSize = FDCAN_DATA_BYTES_8;

			pFilterConfig->IdType = FDCAN_STANDARD_ID;
			pFilterConfig->FilterIndex = 0;
			pFilterConfig->FilterType = FDCAN_FILTER_MASK;
			pFilterConfig->FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
			pFilterConfig->FilterID1 = 0x321;
			pFilterConfig->FilterID2 = 0x7FF;

			RetVal = COMM_SUCCESS;
			break;
		case DRIVER_CFG2:
			/*	Bit time configuration:
				fdcan_ker_ck               = 40 MHz
				Time_quantum (tq)          = 200 ns
				Synchronization_segment    = 1 tq
				Propagation_segment        =  tq
				Phase_segment_1            =  tq
				Phase_segment_2            =  tq
				Synchronization_Jump_width =  tq
				Bit_length                 = 20 tq = 1 �s
				Bit_rate                   = 250 k Bit/s

				sample point at 75 %
			*/
			pHfdcan->Instance = FDCANx;
			pHfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
			pHfdcan->Init.Mode = FDCAN_MODE;
			pHfdcan->Init.AutoRetransmission = ENABLE;
			pHfdcan->Init.TransmitPause = DISABLE;
			pHfdcan->Init.ProtocolException = ENABLE;
			pHfdcan->Init.NominalPrescaler = 0x4; /* tq = NominalPrescaler x (1/fdcan_ker_ck) */
			pHfdcan->Init.NominalSyncJumpWidth = 0x01;
			pHfdcan->Init.NominalTimeSeg1 = 34U; /* NominalTimeSeg1 = Propagation_segment + Phase_segment_1 */
			pHfdcan->Init.NominalTimeSeg2 = 5U;
			pHfdcan->Init.MessageRAMOffset = 0;
			pHfdcan->Init.StdFiltersNbr = 1;
			pHfdcan->Init.ExtFiltersNbr = 0;
			pHfdcan->Init.RxFifo0ElmtsNbr = 1;
			pHfdcan->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
			pHfdcan->Init.RxFifo1ElmtsNbr = 0;
			pHfdcan->Init.RxBuffersNbr = 0;
			pHfdcan->Init.TxEventsNbr = 0;
			pHfdcan->Init.TxBuffersNbr = 0;
			pHfdcan->Init.TxFifoQueueElmtsNbr = 16;
			pHfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
			pHfdcan->Init.TxElmtSize = FDCAN_DATA_BYTES_8;

			pFilterConfig->IdType = FDCAN_STANDARD_ID;
			pFilterConfig->FilterIndex = 0;
			pFilterConfig->FilterType = FDCAN_FILTER_MASK;
			pFilterConfig->FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
			pFilterConfig->FilterID1 = 0x321;
			pFilterConfig->FilterID2 = 0x7FF;

			RetVal = COMM_SUCCESS;
			break;
		default:
			break;
	}

	return RetVal;
}
