/*
 * fdcan.c
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

#include "fdcan.h"
#include "fdcan_utils.h"
#include "nvic_irg_config.h"

#define FDCAN_1_NBR        COMM_DEVICE_NUMBER_1
#define FDCAN_2_NBR        COMM_DEVICE_NUMBER_2

typedef struct {
    FDCAN_GlobalTypeDef *fdcan;
    FDCAN_HandleTypeDef hfdcan;
    FDCAN_RxHeaderTypeDef rxheader;
    uint32_t mostRecentInterrupTimestamp;
} FdcanInstanceType;
 
static FdcanInstanceType fdcan_hfdcan[FDCAN_MAX_INSTANCES] = {
    {
        .fdcan=NULL,
        .mostRecentInterrupTimestamp=0
    },
    {
        .fdcan=NULL,
        .mostRecentInterrupTimestamp=0
    }
};

CommDriverConfigType Can1Cfg;

/* Private function prototypes -----------------------------------------------*/
comm_status_t FDCAN_Ioctl(
    CommDriver *handle, 
    int cmd, 
    void *argument);

comm_status_t fdcan_find_free(FdcanInstanceType **handle)
{
    comm_status_t res = COMM_ERROR;
    
    for (uint8_t i=0; i < FDCAN_MAX_INSTANCES; i++)
    {
        if (NULL == fdcan_hfdcan[i].fdcan)
        {
            *handle = &fdcan_hfdcan[i];
            res = COMM_SUCCESS;
            break;
        }
    }

    return res;
}

comm_status_t fdcan_get_handle(FDCAN_GlobalTypeDef *fdcan, FdcanInstanceType **handle)
{
    if (NULL == fdcan)
    {
        return COMM_ERROR;
    }

    for (uint8_t i=0; i < FDCAN_MAX_INSTANCES; i++)
    {
        if (fdcan == fdcan_hfdcan[i].fdcan)
        {
            *handle = &fdcan_hfdcan[i];
            return COMM_SUCCESS;
        }
    }

    return COMM_ERROR;
}

comm_status_t get_fdcan_config(
        FdcanInstanceType *,
		FDCAN_FilterTypeDef *);

comm_status_t fdcan_init_tx_header(
    const void *, 
    FDCAN_TxHeaderTypeDef *, 
    uint32_t);

comm_status_t SampleTime(
    uint32_t *timestamp);

comm_status_t FDCAN_CreateDriver(
    CommDriver *pDriver, 
    const void *cfg, 
    size_t cfg_size,
    uint8_t *tx, 
    uint8_t *rx)
{
	comm_status_t RetVal;
    FdcanInstanceType * instance;

	RetVal = COMM_ERROR;
    pDriver->config = (CommDriverConfigType *)cfg;

    if (sizeof(CommDriverConfigType) != cfg_size)
    {
        FDCAN_ErrorHandler();
    }

    if (COMM_SUCCESS != fdcan_find_free(&instance))
    {
        RetVal = COMM_ERROR;
        FDCAN_ErrorHandler();
    }

    switch (pDriver->config->devNbr)
    {
    case COMM_DEVICE_NUMBER_1:
        instance->fdcan = FDCAN_1;
        RetVal = COMM_SUCCESS;
        break;
    case COMM_DEVICE_NUMBER_2:
        instance->fdcan = FDCAN_2;
        RetVal = COMM_SUCCESS;
        break;
    default:
        RetVal = COMM_ERROR;
        break;
    }

    if (COMM_SUCCESS != RetVal)
    {
        FDCAN_ErrorHandler();
    }
    
    pDriver->instance = (void *) instance;

	pDriver->interface->init = FDCAN_Init;
	pDriver->interface->send = FDCAN_Send;
	pDriver->interface->read = FDCAN_Read;
    pDriver->interface->ioctl = FDCAN_Ioctl;
	pDriver->state = DRIVER_STATE_UNINITIALIZED;
	pDriver->protocol = DRIVER_FDCAN;
	pDriver->TxFrameBuffer = tx;
    pDriver->RxFrameBuffer = rx;

    pDriver->state = DRIVER_STATE_INITIALIZED;
    RetVal = COMM_SUCCESS;

	return RetVal;
}

COMM_REGISTER_DRIVER(DRIVER_FDCAN, FDCAN_CreateDriver);

comm_status_t FDCAN_Init(
    CommDriver *dev)
{
	FDCAN_FilterTypeDef sFilterConfig;
	comm_status_t RetVal;
    FdcanInstanceType * instance;

    RetVal = COMM_SUCCESS;
    instance = (FdcanInstanceType *) dev->instance;

    // TODO make init function consistent with driver creation
    get_fdcan_config(instance, &sFilterConfig);

    if (HAL_FDCAN_Init(&instance->hfdcan) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    if (HAL_FDCAN_ConfigTimestampCounter(&instance->hfdcan, FDCAN_TIMESTAMP_PRESC_1) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    if (HAL_FDCAN_EnableTimestampCounter(&instance->hfdcan, FDCAN_TIMESTAMP_EXTERNAL) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    /* Configure Rx filter */
    if (HAL_FDCAN_ConfigFilter(&instance->hfdcan, &sFilterConfig) != HAL_OK)
    {
        /* Filter configuration Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    /* Start the FDCAN module */
    // if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
    // {
    //     /* Start Error */
    //     RetVal = COMM_ERROR;
    // }

    if (HAL_FDCAN_ActivateNotification(&instance->hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
        /* Notification Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    /* Prepare Tx Header */

    if (0 == RetVal)
    {
        dev->state = DRIVER_STATE_INITIALIZED;
    }
    return RetVal;
}

comm_status_t FDCAN_DeInit(
    CommDriver *dev)
{
	comm_status_t RetVal;
    FdcanInstanceType * instance;

    RetVal = COMM_SUCCESS;
    instance = (FdcanInstanceType *) dev->instance;

    if (HAL_FDCAN_DeactivateNotification(&instance->hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != HAL_OK)
    {
        /* Notification Error */
        RetVal = COMM_ERROR;
    }

    /* Stop the FDCAN module */
    if (HAL_FDCAN_Stop(&instance->hfdcan) != HAL_OK)
    {
        /* Start Error */
        RetVal = COMM_ERROR;
    }

    if (HAL_FDCAN_DisableTimestampCounter(&instance->hfdcan) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    if (HAL_FDCAN_DeInit(&instance->hfdcan) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    return RetVal;
}

comm_status_t FDCAN_Send(
    CommDriver *dev,
    const void *pMsg)
{
	comm_status_t RetVal;
	uint8_t * pData;
	FDCAN_Message *pMsgCpy;
	FDCAN_TxHeaderTypeDef TxHeader;
    FdcanInstanceType * instance;

    if (DRIVER_STATE_STARTED != dev->state)
    {
        return COMM_INVALID_STATE;
    }

    RetVal = COMM_SUCCESS;
    instance = (FdcanInstanceType *) dev->instance;
    pMsgCpy = (FDCAN_Message*)pMsg;
    pData = (uint8_t*)(pMsgCpy->msgBase.payload);
	
	(void)fdcan_init_tx_header(pMsgCpy, &TxHeader, pMsgCpy->msgBase.length);

	if (HAL_FDCAN_AddMessageToTxFifoQ(&instance->hfdcan, &TxHeader, pData) == HAL_OK)
	{
		RetVal = COMM_SUCCESS;
	}
	else
	{
		RetVal = COMM_ERROR;
	}

	return RetVal;
}

comm_status_t FDCAN_Read(
    CommDriver *dev,
    void *pFrame, 
    uint8_t length, 
    uint32_t RxFifo0ITs)
{
  comm_status_t RetVal;
  uint8_t Data[8];
  FDCAN_ClassicFrame *pNewFrame;
  FdcanInstanceType * instance;

  RetVal = COMM_SUCCESS;
  instance = (FdcanInstanceType *) dev->instance;
  pNewFrame = (FDCAN_ClassicFrame*)pFrame;

  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(&instance->hfdcan, FDCAN_RX_FIFO0, &instance->rxheader, Data) != HAL_OK)
    {
    	/* Reception Error */
    	RetVal = COMM_ERROR;
    }

    /* Display LEDx */
    if ((instance->rxheader.Identifier == 0x321) && (instance->rxheader.IdType == FDCAN_STANDARD_ID) && (instance->rxheader.DataLength == FDCAN_DLC_BYTES_2))
    {

    }

    switch (instance->rxheader.DataLength)
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


    pNewFrame->id = instance->rxheader.Identifier;
    pNewFrame->timestamp = instance->mostRecentInterrupTimestamp; // RxHeader.RxTimestamp;

    memcpy(pNewFrame->data, &Data, pNewFrame->dlc);

    if (HAL_FDCAN_ActivateNotification(&instance->hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
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

void FDCAN_GetMostRecentInterruptTimestamp(CommDriver *dev, uint32_t *timestamp)
{
    *timestamp = ((FdcanInstanceType*)dev->instance)->mostRecentInterrupTimestamp;
}

void FDCAN_1_IRQHandler(void)
{
    FdcanInstanceType * instance;

    if (COMM_SUCCESS != fdcan_get_handle(FDCAN1, &instance))
    {
        FDCAN_ErrorHandler();
    }

    SampleTime((uint32_t *)&instance->mostRecentInterrupTimestamp);
    HAL_FDCAN_IRQHandler(&instance->hfdcan);
}

void FDCAN_2_IRQHandler(void)
{
    FdcanInstanceType * instance;

    if (COMM_SUCCESS != fdcan_get_handle(FDCAN2, &instance))
    {
        FDCAN_ErrorHandler();
    }

    SampleTime((uint32_t *)&instance->mostRecentInterrupTimestamp);
    HAL_FDCAN_IRQHandler(&instance->hfdcan);
}

/* IOCTL/ driver specific functions */

static comm_status_t FDCAN_SetBaudrate(
    CommDriver *dev,
    uint32_t baudrate)
{
    comm_status_t res = 0;
    uint32_t FdcanClock = 0;
    uint8_t timings[4];
    bool IsDataPhase = false;
    uint16_t Prescaler;
    uint8_t Seg1;        
    uint8_t Seg2;        
    uint8_t Sjw;      
    FdcanInstanceType * instance;
    
    instance = (FdcanInstanceType *)dev->instance;
    FdcanClock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN);

    res = COMM_SUCCESS;
    switch (baudrate)
    {
        case FDCAN_BAUDRATE_250000:
        case FDCAN_BAUDRATE_500000:
        case FDCAN_BAUDRATE_1000000:
            break;
        default:
            res = COMM_INVALID_PARAMETER;
            break;
    }

    memset(timings, 0x00, sizeof(timings));

    if (COMM_SUCCESS != res)
    {
    }
    else if (0 == CANFD_CalculateBitTimingRegister(FdcanClock, baudrate, 7500, 1, IsDataPhase, timings))
    {
        res = COMM_SUCCESS;
    }
    else
    {
        res = COMM_ERROR;
    }

    if (COMM_SUCCESS == res)
    {
        Prescaler  = CANFD_GetPrescaler(timings, IsDataPhase);
        Seg1       = CANFD_GetSeg1(timings, IsDataPhase);
        Seg2       = CANFD_GetSeg2(timings, IsDataPhase);
        Sjw        = CANFD_GetSJW(timings, IsDataPhase);

        instance->hfdcan.Init.NominalPrescaler = Prescaler; 
        instance->hfdcan.Init.NominalSyncJumpWidth = Sjw;
        instance->hfdcan.Init.NominalTimeSeg1 = Seg1; 
        instance->hfdcan.Init.NominalTimeSeg2 = Seg2;

        if (HAL_FDCAN_Init(&instance->hfdcan) != HAL_OK)
        {
            /* Initialization Error */
            res = COMM_ERROR;
        }
    }

    return res;
}

static comm_status_t FDCAN_SetMode(
    CommDriver *dev,
    uint32_t mode)
{
    comm_status_t res = 0;
    FdcanInstanceType * instance;
    
    instance = (FdcanInstanceType *)dev->instance;

    switch (mode)
    {
        case FDCAN_MODE_1:
            instance->hfdcan.Init.Mode = FDCAN_MODE_NORMAL; 
            res = COMM_SUCCESS;
            break;
        case FDCAN_MODE_2:
            instance->hfdcan.Init.Mode = FDCAN_MODE_BUS_MONITORING; 
            res = COMM_SUCCESS;
            break;
        default:
            res = COMM_INVALID_PARAMETER;
            break;
    }

    if (COMM_SUCCESS == res)
    {
        if (HAL_FDCAN_Init(&instance->hfdcan) != HAL_OK)
        {
            /* Initialization Error */
            res = COMM_ERROR;
        }
    }

    return res;
}

comm_status_t FDCAN_Ioctl(
    CommDriver *dev, 
    int cmd, 
    void *argument)
{
    comm_status_t res = 0;
    FdcanInstanceType * instance;

    instance = (FdcanInstanceType *) dev->instance;

    switch (cmd)
    {
        case CANABS_IOCTL_CMD_SET_BAUDRATE:
            {
                FdcanBaudrateType baudrate = *((FdcanBaudrateType *)argument);
                res = FDCAN_SetBaudrate(dev, baudrate);
            }
            break;
        case CANABS_IOCTL_CMD_START:
            if (DRIVER_STATE_STARTED == dev->state)
            {
                /* nothing to do */
            }
            else if (0 != instance->hfdcan.ErrorCode)
            {
                res = COMM_ERROR;
                FDCAN_ErrorHandler();
            }
            else if (HAL_FDCAN_Start(&instance->hfdcan) != HAL_OK)
            {
                /* Start Error */
                res = COMM_ERROR;
                FDCAN_ErrorHandler();
            }
            else
            {
                dev->state = DRIVER_STATE_STARTED;
            }
            break;
        case CANABS_IOCTL_CMD_STOP:
            if (DRIVER_STATE_STARTED != dev->state)
            {
                /* nothing to do */
            }
            else if (HAL_FDCAN_Stop(&instance->hfdcan) != HAL_OK)
            {
                /* Start Error */
                res = COMM_ERROR;
                FDCAN_ErrorHandler();
            }
            else 
            {
                dev->state = DRIVER_STATE_STOPPED;
            }
            break;
        case CANABS_IOCTL_CMD_SET_MODE:
            {
                FdcanModeType mode = *((FdcanModeType *)argument);
                res = FDCAN_SetMode(dev, mode);

                if (COMM_SUCCESS == res && mode == FDCAN_MODE_3)
                {
                    dev->state = DRIVER_STATE_OFF;
                }
            }
            break;
            break;
        case CANABS_IOCTL_CMD_SET_FILTERMASK:
        default:
            res = 1;
            break;
    }    

    return res;
}

static inline void gpio_clk_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA) 
    { 
        __HAL_RCC_GPIOA_CLK_ENABLE(); 
    }
    else if (port == GPIOB) 
    { 
        __HAL_RCC_GPIOB_CLK_ENABLE(); 
    }
    else if (port == GPIOC) 
    { 
        __HAL_RCC_GPIOC_CLK_ENABLE(); 
    }
    else if (port == GPIOD) 
    { 
        __HAL_RCC_GPIOD_CLK_ENABLE(); 
    }
    else if (port == GPIOE) 
    { 
        __HAL_RCC_GPIOE_CLK_ENABLE(); 
    }
#ifdef GPIOF
    else if (port == GPIOF) 
    { 
        __HAL_RCC_GPIOF_CLK_ENABLE(); 
    }
#endif
#ifdef GPIOG
    else if (port == GPIOG) 
    { 
        __HAL_RCC_GPIOG_CLK_ENABLE(); 
    }
#endif
#ifdef GPIOH
    else if (port == GPIOH) 
    { 
        __HAL_RCC_GPIOH_CLK_ENABLE(); 
    }
#endif
}

HAL_StatusTypeDef FDCAN_GpioClck(FDCAN_GlobalTypeDef *fdcan)
{
    HAL_StatusTypeDef res;

    res = HAL_OK;

    if (FDCAN_1 == fdcan)
    {
        CLK_ENABLE(FDCAN_1_TX_GPIO_PORT);
        CLK_ENABLE(FDCAN_1_RX_GPIO_PORT);
    }
    else if (FDCAN_2 == fdcan)
    {
        CLK_ENABLE(FDCAN_2_TX_GPIO_PORT);
        CLK_ENABLE(FDCAN_2_RX_GPIO_PORT);
    }
    else
    {
        res = HAL_ERROR;
    }

    return res;
}

HAL_StatusTypeDef FDCAN_InitGpio(FDCAN_GlobalTypeDef *fdcan)
{
    HAL_StatusTypeDef res;

    res = HAL_OK;

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;

    if (FDCAN_1 == fdcan)
    {
        GPIO_InitStruct.Pin       = FDCAN_1_TX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_1_TX_AF;
        (void) HAL_GPIO_Init(FDCAN_1_TX_GPIO_PORT, &GPIO_InitStruct);
    
        /* FDCANx RX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = FDCAN_1_RX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_1_RX_AF;
        (void) HAL_GPIO_Init(FDCAN_1_RX_GPIO_PORT, &GPIO_InitStruct);
    }
    else if (FDCAN_2 == fdcan)
    {
        GPIO_InitStruct.Pin       = FDCAN_2_TX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_2_TX_AF;
        (void) HAL_GPIO_Init(FDCAN_2_TX_GPIO_PORT, &GPIO_InitStruct);
    
        /* FDCANx RX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = FDCAN_2_RX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_2_RX_AF;
        (void) HAL_GPIO_Init(FDCAN_2_RX_GPIO_PORT, &GPIO_InitStruct);
    }
    else
    {
        res = HAL_ERROR;
    }

    return res;
}

HAL_StatusTypeDef FDCAN_Nvic(FDCAN_GlobalTypeDef *fdcan)
{
    HAL_StatusTypeDef res;

    res = HAL_OK;

    if (FDCAN_1 == fdcan)
    {
        HAL_NVIC_SetPriority(FDCAN_1_IRQn, FDCAN_IRQ_PREEMPT_PRIO, 1);
        HAL_NVIC_EnableIRQ(FDCAN_1_IRQn);
    }
    else if (FDCAN_2 == fdcan)
    {
        HAL_NVIC_SetPriority(FDCAN_2_IRQn, FDCAN_IRQ_PREEMPT_PRIO, 1);
        HAL_NVIC_EnableIRQ(FDCAN_2_IRQn);
    }
    else
    {
        res = HAL_ERROR;
    }

    return res;
}

/**
  * @brief  Initializes the FDCAN MSP.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @retval None
  */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* hfdcan)
{
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    (void) FDCAN_GpioClck(hfdcan->Instance);

    /* Select PLL1Q as source of FDCANx clock */
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    RCC_PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
    (void) HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    /* Enable FDCANx clock */
    FDCANx_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* FDCANx TX GPIO pin configuration  */
    (void) FDCAN_InitGpio(hfdcan->Instance);

    /*##-3- Configure the NVIC #################################################*/
    /* NVIC for FDCANx */
    (void) FDCAN_Nvic(hfdcan->Instance);
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
    if (FDCAN_1 == hfdcan->Instance)
    {
        /* Configure FDCANx Tx as alternate function  */
        HAL_GPIO_DeInit(FDCAN_1_TX_GPIO_PORT, FDCAN_1_TX_PIN);

        /* Configure FDCANx Rx as alternate function  */
        HAL_GPIO_DeInit(FDCAN_1_RX_GPIO_PORT, FDCAN_1_RX_PIN);

        /*##-3- Disable the NVIC for FDCANx ########################################*/
        HAL_NVIC_DisableIRQ(FDCAN_1_IRQn);
    }
    else if (FDCAN_2 == hfdcan->Instance)
    {
        HAL_GPIO_DeInit(FDCAN_2_TX_GPIO_PORT, FDCAN_2_TX_PIN);
        HAL_GPIO_DeInit(FDCAN_2_RX_GPIO_PORT, FDCAN_2_RX_PIN);
        HAL_NVIC_DisableIRQ(FDCAN_2_IRQn);
    }
    else
    {
        FDCAN_ErrorHandler();
    }
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

comm_status_t ram_usage(FDCAN_GlobalTypeDef *fdcan, uint32_t *size)
{
    comm_status_t res;

    res = COMM_SUCCESS;

    if (FDCAN1 == fdcan)
    {
        *size = 0U * (FDCAN_RAM_RX_SECTION_SIZE);
    }
    else if (FDCAN2 == fdcan)
    {
        *size = 1U * (FDCAN_RAM_RX_SECTION_SIZE);
    }
    else
    {
        res = COMM_ERROR;
    }

    return res;
}

comm_status_t get_fdcan_config(
        FdcanInstanceType * instance,
		FDCAN_FilterTypeDef *pFilterConfig)
{
	comm_status_t RetVal;
	RetVal = COMM_ERROR;

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
    instance->hfdcan.Instance = instance->fdcan;
    instance->hfdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    instance->hfdcan.Init.Mode = FDCAN_MODE_DEFAULT;
    instance->hfdcan.Init.AutoRetransmission = ENABLE;
    instance->hfdcan.Init.TransmitPause = DISABLE;
    instance->hfdcan.Init.ProtocolException = ENABLE;
    instance->hfdcan.Init.NominalPrescaler = 0x4; /* tq = NominalPrescaler x (1/fdcan_ker_ck) */
    instance->hfdcan.Init.NominalSyncJumpWidth = 0x01;
    instance->hfdcan.Init.NominalTimeSeg1 = 34U; /* NominalTimeSeg1 = Propagation_segment + Phase_segment_1 */
    instance->hfdcan.Init.NominalTimeSeg2 = 5U;
    ram_usage(instance->fdcan, &instance->hfdcan.Init.MessageRAMOffset);
    instance->hfdcan.Init.StdFiltersNbr = 1;
    instance->hfdcan.Init.ExtFiltersNbr = 0;
    instance->hfdcan.Init.RxFifo0ElmtsNbr = FDCAN_RAM_RX_ELEMENTS;
    instance->hfdcan.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    instance->hfdcan.Init.RxFifo1ElmtsNbr = 0;
    instance->hfdcan.Init.RxBuffersNbr = 0;
    instance->hfdcan.Init.TxEventsNbr = 0;
    instance->hfdcan.Init.TxBuffersNbr = 0;
    instance->hfdcan.Init.TxFifoQueueElmtsNbr = 4;
    instance->hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    instance->hfdcan.Init.TxElmtSize = FDCAN_DATA_BYTES_8;

    pFilterConfig->IdType = FDCAN_STANDARD_ID;
    pFilterConfig->FilterIndex = 0;
    pFilterConfig->FilterType = FDCAN_FILTER_MASK;
    pFilterConfig->FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    pFilterConfig->FilterID1 = 0x321;
    pFilterConfig->FilterID2 = 0x7FF;

    RetVal = COMM_SUCCESS;

	return RetVal;
}

comm_status_t SampleTime(uint32_t *timestamp)
{
    comm_status_t res = 0;
    uint64_t time;

    (void) FDCAN_GetTimestamp(&time);

    memcpy((uint8_t*)timestamp, (uint8_t*)&time, sizeof(*timestamp));

    return res;
}
