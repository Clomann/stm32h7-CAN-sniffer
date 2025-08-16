#include <assert.h>
#include <stdio.h>

#include "spi.h"
#include "Spi_Cmds.h"

#if 0
/* Buffer used for transmission */
ALIGN_32BYTES(
    uint8_t __attribute__((section(".dma_buffer")))
    aTxBuffer[SPI_MAX_INSTANCES][SPI_TX_RING_SIZE]
             [DRV_BUFFER_ALIGNED_SIZE(SPI_TX_BUFFER_SIZE)]
);

/* Buffer used for reception */
/* Size of buffer */
ALIGN_32BYTES(
    uint8_t __attribute__((section(".dma_buffer")))
    aRxBuffer[SPI_MAX_INSTANCES][SPI_RX_RING_SIZE]
             [DRV_BUFFER_ALIGNED_SIZE(SPI_RX_BUFFER_SIZE)]
);

_Static_assert(SPI_TX_RING_SIZE >= 2, "Need at least ping-pong (2) TX buffers");
_Static_assert(
    (SPI_BUFFER_ALIGNED_SIZE(SPI_TX_BUFFER_SIZE) & 31U) == 0,
    "Aligned size must be multiple of 32 bytes"
);

#endif

_Static_assert(
    offsetof(SpiSlotType, data) == SPI_SLOT_META_BYTES,
    "`SPI_SLOT_META_BYTES` must be equal to the summed size of all data before "
    "`data` in `SpiSlotType`"
);

_Static_assert(
    offsetof(SpiSlotType, data) == sizeof(SpiSlotType),
    "`data` must be the last member of SpiSlotType"
);

static SpiInstanceType spi_instances[SPI_MAX_INSTANCES] = {
    {.spi = NULL, .hspi = {0}, .drv = NULL},
    {.spi = NULL, .hspi = {0}, .drv = NULL}
};

static CommInterface SpiInterface;

comm_status_t m_find_free(SpiInstanceType **handle)
{
    comm_status_t res = COMM_ERROR;

    for (uint8_t i = 0; i < SPI_MAX_INSTANCES; i++)
    {
        if (NULL == spi_instances[i].spi)
        {
            *handle = &spi_instances[i];
            res     = COMM_SUCCESS;
            break;
        }
    }

    return res;
}

comm_status_t SPI_Init(CommDriver *drv)
{
    comm_status_t res;
    SpiInstanceType *instance;
    SpiConfigType *cfg;

    res = COMM_SUCCESS;

    instance = (SpiInstanceType *)drv->instance;
    cfg      = (SpiConfigType *)(drv->config->driver);

    (void)cfg;

    res = (comm_status_t)Spi_Init((SPI_HandleTypeDef *)&instance->hspi);

    if (0 == res)
    {
        drv->state = DRIVER_STATE_INITIALIZED;
    }
    return res;
}

comm_status_t SPI_DeInit(CommDriver *drv)
{
    comm_status_t res = 0;

    (void)res;
    (void)drv;

    return res;
}

static void m_AssignCheckTransactionWithFallback(
    SpiTransactionType *target,
    const SpiTransactionType *source
)
{
    if (NULL == source)
    {
        memset(target, 0, sizeof(SpiTransactionType));
        target->direction = SPI_DIR_TX_RX;
        target->timeout   = 1000;
    }
    else
    {
        *target = *source;
    }
}

static comm_status_t m_PrepareTxSlot(
    CommDriver *drv,
    const SPI_Message *pMsg,
    SpiDirectionType direction,
    SpiSlotType **out_slot
)
{
    comm_status_t res;
    RingBuffer *txSlots;
    SpiSlotType *txSlot;

    res = COMM_SUCCESS;

    if (drv == NULL || drv->TxFrameBuffer == NULL)
    {
        res = COMM_NULL_POINTER;
        SPI_ErrorHandler();
        return res;
    }

    txSlots = (RingBuffer *)Spi_GetSlots(drv->TxFrameBuffer);
    txSlot  = (SpiSlotType *)ring_buffer_reserve(txSlots);

    if (txSlot == NULL)
    {
        SPI_ErrorHandler();
        res = COMM_NULL_POINTER;
        return res;
    }

    m_AssignCheckTransactionWithFallback(
        &txSlot->transaction,
        (SpiTransactionType *)pMsg->transaction
    );

    /* Tx/Rx is kept here for compatibility with functions in SD.c */
    txSlot->transaction.direction = direction;
    txSlot->transaction.length    = pMsg->msgBase.length;

    txSlot->used_len = pMsg->msgBase.length;

    *out_slot = txSlot;

    return res;
}

comm_status_t SPI_Send(CommDriver *drv, const void *msg)
{
    comm_status_t res;
    SPI_Message *pMsg;
    SpiSlotType *txSlot;

    pMsg = (SPI_Message *)msg;

    res = m_PrepareTxSlot(drv, pMsg, SPI_DIR_TX_RX, &txSlot);
    if (res != COMM_SUCCESS)
    {
        return res;
    }

    memcpy(txSlot->data, pMsg->msgBase.payload, txSlot->used_len);

    return res;
}

comm_status_t SPI_Read(
    CommDriver *drv,
    void *msg,
    uint8_t length, /* for comapitibilty (fdcan) */
    uint32_t RxFifo0ITs /* for comapitibilty (fdcan) */
)
{
    SPI_Message *pMsg;
    SpiSlotType *txSlot;

    (void)length;
    (void)RxFifo0ITs;

    pMsg = (SPI_Message *)msg;

    return m_PrepareTxSlot(drv, pMsg, SPI_DIR_RX_ONLY, &txSlot);
}

comm_status_t SPI_Ioctl(CommDriver *dev, int cmd, void *argument)
{
    comm_status_t res = 0;
    SpiInstanceType *instance;

    instance = (SpiInstanceType *)dev->instance;

    (void)dev;
    (void)cmd;
    (void)argument;
    (void)instance;
    (void)res;

    return res;
}

comm_status_t SPI_DestroyDriver(CommDriver *drv)
{
    SpiInstanceType *instance = (SpiInstanceType *)drv->instance;
    if (instance)
    {
        instance->spi = NULL; // Mark as free
        instance->drv = NULL;
        memset(&instance->hspi, 0, sizeof(SPI_HandleTypeDef));
    }
    drv->instance = NULL;
    return COMM_SUCCESS;
}

comm_status_t SPI_CreateDriver(
    CommDriver *drv,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx
)
{
    comm_status_t res = COMM_ERROR;
    SpiInstanceType *instance;

    drv->config = (CommDriverConfigType *)cfg;

    if (sizeof(CommDriverConfigType) != cfg_size)
    {
        SPI_ErrorHandler();
    }

    if (COMM_SUCCESS != m_find_free(&instance))
    {
        res = COMM_NO_RESSOURCES;
        SPI_ErrorHandler();
        return res;
    }

    // STMs HAL only works on SPI_HandleTypeDef
    // keep ef back to driver to be able to extract
    // transaction info upon transfer completion
    instance->drv = drv;

    switch (drv->config->devNbr)
    {
    case COMM_DEVICE_NUMBER_1:
        instance->spi = SPI_1;

        res = COMM_SUCCESS;
        break;
    case COMM_DEVICE_NUMBER_2:
        instance->spi = SPI_2;

        res = COMM_SUCCESS;
        break;
    default:
        res = COMM_INVALID_PARAMETER;
        break;
    }

    if (COMM_SUCCESS != res)
    {
        SPI_ErrorHandler();
        return res;
    }

    drv->instance = (void *)instance;

    drv->interface        = &SpiInterface;
    drv->interface->init  = SPI_Init;
    drv->interface->send  = SPI_Send;
    drv->interface->read  = SPI_Read;
    drv->interface->ioctl = SPI_Ioctl;
    drv->state            = DRIVER_STATE_UNINITIALIZED;
    drv->protocol         = DRIVER_SPI;
    drv->TxFrameBuffer    = tx;
    drv->RxFrameBuffer    = rx;

    drv->state = DRIVER_STATE_INITIALIZED;

    res = COMM_SUCCESS;

    return res;
}

COMM_REGISTER_DRIVER(DRIVER_SPI, SPI_CreateDriver);

__attribute__((weak)) void SPI_ErrorHandler(void)
{
    ;
}

void SPI_InitTask()
{
}

ALIGN_32BYTES(volatile uint8_t dummy[1024]) = {0};

void SPI_Poll(CommDriver *drv)
{
    comm_status_t res = COMM_ERROR;
    SpiInstanceType *instance;

    RingBuffer *txSlots;
    SpiSlotType *txSlot;
    RingBuffer *rxSlots;
    SpiSlotType *rxSlot;

    SpiTransactionType *Transaction;

    if (drv == NULL || NULL == drv->instance || drv->TxFrameBuffer == NULL
        || drv->RxFrameBuffer == NULL)
    {
        res = COMM_NULL_POINTER;
        SPI_ErrorHandler();
        return;
    }

    instance = (SpiInstanceType *)drv->instance;
    txSlots  = (RingBuffer *)Spi_GetSlots(drv->TxFrameBuffer);
    rxSlots  = (RingBuffer *)Spi_GetSlots(drv->RxFrameBuffer);

    // get oldest tx slot
    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 0);

    if (txSlot == NULL)
    {
        SPI_ErrorHandler();
        res = COMM_NULL_POINTER;
        return;
    }

    rxSlot = ring_buffer_reserve(rxSlots);

    if (rxSlot == NULL)
    {
        SPI_ErrorHandler();
        res = COMM_NULL_POINTER;
        return;
    }

    Transaction = &txSlot->transaction;

    rxSlot->transaction = *Transaction;
    rxSlot->used_len    = Transaction->length;

    switch (Transaction->direction)
    {
    case SPI_DIR_TX_ONLY:
        // length = bytes to transmit
        res = Spi_Send(&instance->hspi, txSlot->data, Transaction->length);
        break;

    case SPI_DIR_RX_ONLY:
        // length = bytes to receive
        res = Spi_Receive(&instance->hspi, rxSlot->data, Transaction->length);
        break;

    case SPI_DIR_TX_RX:
        // length = bytes for both TX and RX (typical SPI)
        res = Spi_SendReceiveMsg(
            &instance->hspi,
            (uint8_t *)txSlot->data,
            (uint8_t *)rxSlot->data,
            Transaction->length
        );
        break;
    default:
        res = SPI_E_NOT_OK;
        SPI_ErrorHandler();
        break;
    }

    if (COMM_SUCCESS != res)
    {
        SPI_ErrorHandler();
    }

    // remove oldest tx slot
    ring_buffer_pop_ptr(txSlots);
}

CommDriver *m_FindCorrespondingDriver(SPI_HandleTypeDef *hspi)
{
    uint32_t index;
    CommDriver *pDrv;
    SpiInstanceType *pInstance;

    pDrv = NULL;

    for (index = 0; index < SPI_MAX_INSTANCES; index++)
    {
        pInstance = &spi_instances[index];

        if (NULL == pInstance)
        {
        }
        else if (&pInstance->hspi == hspi)
        {
            pDrv = pInstance->drv;
        }
    }

    return pDrv;
}

uint8_t Spi_NotifyRxData(SPI_HandleTypeDef *hspi, uint8_t err)
{
    uint8_t res            = 0;
    CommDriver *pDrv       = NULL;
    RingBuffer *rxSlots    = NULL;
    SpiSlotType *rxSlot    = {0};
    SpiSlotType rxSlotCopy = {0};

    if (NULL == hspi)
    {
        res = COMM_INVALID_PARAMETER;
    }

    pDrv = m_FindCorrespondingDriver(hspi);

    if (NULL == pDrv)
    {
        res = COMM_NO_RESSOURCES;
        SPI_ErrorHandler();
        return res;
    }

    rxSlots = (RingBuffer *)Spi_GetSlots(pDrv->RxFrameBuffer);

    rxSlot = (SpiSlotType *)ring_buffer_peek_at(rxSlots, 0);

    if (NULL == rxSlot)
    {
        res = COMM_ERROR;
    }

    if (0 != res)
    {
        // no slot available
    }
    else if (rxSlot->transaction.callback != NULL)
    {
        // Async with callback
        res = ring_buffer_pop(rxSlots, &rxSlotCopy);
        if (COMM_SUCCESS == res)
        {
            rxSlot->transaction.callback(
                rxSlot->transaction.context,
                err,
                rxSlot->data,
                rxSlot->used_len
            );
        }
    }
    else if (rxSlot->transaction.callback == NULL)
    {
        // pop but don't call callback
        (void)ring_buffer_pop_ptr(rxSlots);
    }
    else
    {
        // synchronous, caller has to free buffer
    }

    return res;
}

RingBuffer *Spi_GetSlots(const uint8_t *const buf)
{
    SpiBinType *txBins;

    if (NULL == buf)
    {
        SPI_ErrorHandler();
        return NULL;
    }

    txBins = (SpiBinType *)buf;

    return (RingBuffer *)txBins[0].slots;
}

uint32_t Spi_HasPendingTransfers(CommDriver *drv)
{
    RingBuffer *txSlots;

    if (drv == NULL || drv->TxFrameBuffer == NULL)
    {
        return false;
    }

    txSlots = (RingBuffer *)Spi_GetSlots(drv->TxFrameBuffer);

    // Check if there are any messages in the TX buffer
    return txSlots->elementCount;
}
