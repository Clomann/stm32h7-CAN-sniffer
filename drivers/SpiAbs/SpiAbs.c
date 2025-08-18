#include "SpiAbs.h"
#include "Spi_Cmds.h"
#include "spi.h"

#define SPI_RX_SLOT_REQUIRED_SIZE   (515) /* max payload: token + sector + crc = 1 + 512 + 2 = 515 */
#define SPI_TX_SLOT_REQUIRED_SIZE   (515)
#define  SW_RX_SLOT_COUNT (3U)
#define  SW_TX_SLOT_COUNT (3U)
#define  SW_TX_BIN_COUNT (SPI_PRIORITYn)
#define  SW_RX_BIN_COUNT (SW_TX_BIN_COUNT)

/* SPI 1 */

#define SPI_TX_SLOT_SIZE DRV_BUFFER_ALIGNED_SIZE(SPI_SLOT_META_BYTES + SPI_TX_SLOT_REQUIRED_SIZE)
#define SPI_RX_SLOT_SIZE DRV_BUFFER_ALIGNED_SIZE(SPI_SLOT_META_BYTES + SPI_RX_SLOT_REQUIRED_SIZE)

DRV_ALIGNED_ARRAY(Spi1RxBins[SW_RX_BIN_COUNT][SW_RX_SLOT_COUNT], SPI_RX_SLOT_SIZE);
DRV_ALIGNED_ARRAY(Spi1TxBins[SW_TX_BIN_COUNT][SW_TX_SLOT_COUNT], SPI_TX_SLOT_SIZE);

SPI_ASSERT_SIZE(Spi1TxBins[0][0]);
SPI_ASSERT_ALIGNMENT(Spi1TxBins);

SPI_ASSERT_SIZE(Spi1RxBins[0][0]);
SPI_ASSERT_ALIGNMENT(Spi1RxBins);

static SpiConfigType Spi1Cfg = {
    .rx_bin_cnt = SW_RX_BIN_COUNT,
    .rx_slots = &Spi1RxBins[0][0][0],
    .rx_slots_cnt = SW_RX_SLOT_COUNT,
    .rx_slot_stride = SPI_RX_SLOT_SIZE,
    .tx_bin_cnt = SW_TX_BIN_COUNT,
    .tx_slots = &Spi1TxBins[0][0][0],
    .tx_slots_cnt = SW_TX_SLOT_COUNT,
    .tx_slot_stride = SPI_TX_SLOT_SIZE
};

static RingBuffer Spi1TxSlotBuffer[SW_TX_BIN_COUNT] = {
    { 
        .startAddress = &Spi1TxBins[0][0][0],
        .head = 0,
        .tail = 0,
        .bufferLength = SW_TX_SLOT_COUNT,
        .elementSize = sizeof(SpiSlotType),
        .stride = SPI_TX_SLOT_SIZE,
        .elementCount = 0,
        .isFull = false
    },
    {
        .startAddress = &Spi1TxBins[1][0][0],
        .head = 0,
        .tail = 0,
        .bufferLength = SW_TX_SLOT_COUNT,
        .elementSize = sizeof(SpiSlotType),
        .stride = SPI_TX_SLOT_SIZE,
        .elementCount = 0,
        .isFull = false
    }
};

static SpiBinType Spi1TxBinsRingBuffer[SW_TX_BIN_COUNT] = {
    {
        .slot_cnt = SW_TX_SLOT_COUNT,
        .slots = &Spi1TxSlotBuffer[0]
    },
    {
        .slot_cnt = SW_TX_SLOT_COUNT,
        .slots = &Spi1TxSlotBuffer[1]
    }
};


static RingBuffer Spi1RxSlotBuffer[SW_RX_BIN_COUNT] = {
    { 
        .startAddress = &Spi1RxBins[0][0][0],
        .head = 0,
        .tail = 0,
        .bufferLength = SW_RX_SLOT_COUNT,
        .elementSize = sizeof(SpiSlotType),
        .stride = SPI_RX_SLOT_SIZE,
        .elementCount = 0,
        .isFull = false
    },
    {
        .startAddress = &Spi1RxBins[1][0][0],
        .head = 0,
        .tail = 0,
        .bufferLength = SW_RX_SLOT_COUNT,
        .elementSize = sizeof(SpiSlotType),
        .stride = SPI_RX_SLOT_SIZE,
        .elementCount = 0,
        .isFull = false
    }
};

static SpiBinType Spi1RxBinsRingBuffer[SW_RX_BIN_COUNT] = {
    {
        .slot_cnt = SW_RX_SLOT_COUNT,
        .slots = &Spi1RxSlotBuffer[0]
    },
    {
        .slot_cnt = SW_RX_SLOT_COUNT,
        .slots = &Spi1RxSlotBuffer[1]
    }
};

static CommDriver Spi1Driver;
static CommDriverConfigType Spi1Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_1,
    .driver = (void *)&Spi1Cfg
};

int SpiAbs_Init(CommDriver *dev, CommDriverConfigType *cfg, uint8_t *tx, uint8_t *rx);

uint8_t SpiAbs_SendWithCallback(
    enum SPIABS_DEVICE dev, 
    SpiTransactionType *transaction, 
    const uint8_t * pTxBuffer, 
    uint8_t TxBytes);

static CommDriver * m_GetDriver(enum SPIABS_DEVICE dev)
{
    CommDriver *pDrv;
    
    switch(dev)
    {
        case SPIABS_DEVICE_1:
            pDrv = &Spi1Driver;
            break;
        default:
            pDrv = NULL;
            break;
    }

    return pDrv;
}

static SPI_HandleTypeDef * m_GetHandle(enum SPIABS_DEVICE dev)
{
    SPI_HandleTypeDef * hdl;

    switch (dev)
    {
        case SPIABS_DEVICE_1:
            hdl = (SPI_HandleTypeDef *)( &((SpiInstanceType *)Spi1Driver.instance)->hspi );
            break;
        default:
            hdl = NULL;
    }

    return hdl;
}

void Spi_ErrorHandler()
{
    SpiAbs_ErrorHandler();
}

void * SpiAbs_GetHandle_Spi1()
{
    SPI_HandleTypeDef *hdl;

    hdl = m_GetHandle(SPIABS_DEVICE_1);

    return (void*)hdl;
}

uint8_t SpiAbs_Init_Spi1()
{
    uint8_t res;
    
    res = SpiAbs_Init(&Spi1Driver, &Spi1Config, (uint8_t *)Spi1TxBinsRingBuffer, (uint8_t *)Spi1RxBinsRingBuffer);

    return res;
}

uint8_t SpiAbs_readByte(enum SPIABS_DEVICE dev, uint8_t * resp)
{
    SPI_HandleTypeDef * hdl;

    hdl = m_GetHandle(dev);
    
    if (NULL == hdl)
    {
        return HAL_ERROR;
    }

    return Spi_readByte(hdl, resp);
}

uint8_t SpiAbs_writByte(enum SPIABS_DEVICE dev, const uint8_t *data)
{
    SPI_HandleTypeDef * hdl;

    hdl = m_GetHandle(dev);
    
    if (NULL == hdl)
    {
        return HAL_ERROR;
    }

    return Spi_writByte(hdl, data);   
}

uint8_t SpiAbs_SendWithCallback(
    enum SPIABS_DEVICE dev, 
    SpiTransactionType *transaction, 
    const uint8_t * pTxBuffer, 
    uint8_t TxBytes)
{
    uint8_t res;
    CommDriver * pDrv;
    SPI_HandleTypeDef * hdl;
    SPI_Message Msg = {0};

    hdl = m_GetHandle(dev);
    
    if (NULL == hdl)
    {
        return HAL_ERROR;
    }
    
    pDrv = m_GetDriver(dev);

    if (NULL == pDrv)
    { 
        return COMM_INVALID_PARAMETER;
    }

    Msg.msgBase.protocol = DRIVER_SPI;
    Msg.msgBase.payload = pTxBuffer;
    Msg.msgBase.length = TxBytes;
    Msg.transaction = (void *)transaction;

    res = SPI_Send(pDrv, &Msg);

#if SPI_USE_RTOS
    // notify SPI task to check for new messages
    SpiAbs_TaskSendReceiveCallback();
#else
    // Process immediately in bare metal mode
    SPI_Poll(pDrv);
#endif

    return res;
}

typedef struct {
    uint8_t *data;
    uint16_t length;
} SpiSendReceiveContextType;

/* Dummy function so that SPI driver doesnt drop the Rx slot internally  */
void SpiAbs_SendReceiveMsgCallback(
    void *context,
    uint32_t status,
    const uint8_t *rx_data,
    uint32_t len
)
{
    uint32_t CopyLength = 0;
    SpiSendReceiveContextType *pContext;

    pContext = (SpiSendReceiveContextType *) context;

    if (NULL != pContext && NULL != rx_data)
    {
        CopyLength = (len < pContext->length) ? len : pContext->length;
        memcpy(pContext->data, (uint8_t *)rx_data, CopyLength);
    }
}

uint8_t SpiAbs_SendReceiveMsg(enum SPIABS_DEVICE dev, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes)
{
    uint8_t res;
    CommDriver * pDrv;
    SPI_HandleTypeDef * hdl;
    SPI_Message Msg = {0};
    SpiTransactionType Transaction = {0};
    SpiSendReceiveContextType Context;

    hdl = m_GetHandle(dev);
    
    if (NULL == hdl)
    {
        return HAL_ERROR;
    }
    
    pDrv = m_GetDriver(dev);

    if (NULL == pDrv)
    { 
        return COMM_INVALID_PARAMETER;
    }

    Context.data = pRxBuffer;
    Context.length = TxBytes;

    Transaction.prio = SPI_PRIORITY_LOW;
    Transaction.length = TxBytes;
    Transaction.callback = SpiAbs_SendReceiveMsgCallback;
    Transaction.context = &Context;

    Msg.transaction = &Transaction;
    Msg.msgBase.protocol = DRIVER_SPI;
    Msg.msgBase.payload = pTxBuffer;
    Msg.msgBase.length = TxBytes;

    res = SPI_Send(pDrv, &Msg);

    if (COMM_SUCCESS != res )
    {
        return res;
    }

    // TODO: handle in caller/ task context    
    SPI_Poll(pDrv);

    // from here omn the hardware takes control
    // and calls `Spi_NotifyRxData` on completion
    // which calls the callers callback from the `transaction`
    // if it was set
    return res;
}

typedef struct {
    uint8_t status;
    uint8_t done;
} TaskContextType;

void SpiAbs_Send_Spi1_CompleteCallback_Task0(void * context, uint32_t status, const uint8_t *data, uint32_t len)
{
    TaskContextType *pTaskContext;
    (void) status;
    (void) data;
    (void) len;

    if (NULL != context)
    {
        pTaskContext = (TaskContextType *)context;
        pTaskContext->status = status;
        pTaskContext->done = 1;
    }
    else
    {
        SpiAbs_ErrorHandler();
    }
}

void SpiAbs_Send_Spi1_Task0(const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes)
{
    SpiTransactionType transaction = {
        .callback = NULL,
        .context = NULL
    };
    TaskContextType context = {
        .done = 0,
        .status = 0   
    };

    transaction.id = 1;
    transaction.prio = SPI_PRIORITY_LOW;
    transaction.timeout = 100;
    transaction.callback = SpiAbs_Send_Spi1_CompleteCallback_Task0;
    transaction.context = (void *)&context;

    SpiAbs_SendWithCallback(SPIABS_DEVICE_1, &transaction, pTxBuffer, TxBytes);

    while (1 != context.done)
    {
        
    }
}

uint8_t SpiAbs_PollForResponse(enum SPIABS_DEVICE dev, uint8_t * pResponse)
{
    SPI_HandleTypeDef * hdl;

    hdl = m_GetHandle(dev);
    
    if (NULL == hdl)
    {
        return HAL_ERROR;
    }

    return Spi_PollForResponse(hdl, pResponse);
}

uint8_t SpiAbs_PwrOn(enum SPIABS_DEVICE dev)
{
    if (SPIABS_DEVICE_1 == dev)
    {
        return Spi_PwrOn();
    }

    return HAL_ERROR;
}

uint8_t SpiAbs_PwrOff(enum SPIABS_DEVICE dev)
{
    if (SPIABS_DEVICE_1 == dev)
    {
        return Spi_PwrOff();
    }

    return HAL_ERROR;
}

uint8_t SpiAbs_CsEnable(enum SPIABS_DEVICE dev)
{
    if (SPIABS_DEVICE_1 == dev)
    {
        return Spi_CsEnable();
    }

    return HAL_ERROR;
}

uint8_t SpiAbs_CsDisable(enum SPIABS_DEVICE dev)
{
    if (SPIABS_DEVICE_1 == dev)
    {
        return Spi_CsDisable();
    }

    return HAL_ERROR;
}

int SpiAbs_Init(CommDriver *dev, CommDriverConfigType *cfg, uint8_t *tx, uint8_t *rx)
{
    unsigned int res = COMM_SUCCESS;

    dev->protocol = DRIVER_SPI;

    memset(Spi1TxBins, 0x00, sizeof(Spi1TxBins));
    memset(Spi1RxBins, 0x00, sizeof(Spi1RxBins));

    (void)CommManager_Init(dev, (const void *)cfg, sizeof(CommDriverConfigType), tx, rx);

	if (dev->interface->init(dev) != COMM_SUCCESS)
	{
	  res = 1;
	}

    return res;
}

uint8_t SpiAbs_Poll(void)
{
    bool work_done;

    work_done = false;

    if (Spi_HasPendingTransfers(&Spi1Driver)) {
        SPI_Poll(&Spi1Driver);
    
        work_done = true;
    }

    return work_done;
}

void SpiAbs_Task(void *parameters)
{
    SpiAbs_TaskControlCallback(0);

    while (1)
    {
        SpiAbs_TaskControlCallback(0xFFFFFFFFUL);
        
        SpiAbs_Poll();
    }
}

void __attribute__((weak))  SpiAbs_TaskControlCallback(uint32_t timeout)
{
    (void) timeout;
}

void __attribute__((weak)) SpiAbs_TaskSendReceiveCallback()
{
    ;
}