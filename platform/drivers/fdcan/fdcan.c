/*
 * fdcan.c
 *
 *  Created on: 14.09.2024
 *      Author: Clemens
 */

#include "fdcan.h"
#include "CommTypes.h"
#include "fdcan_utils.h"
#include "nvic_irg_config.h"
#include "stm32h7xx_hal_fdcan.h"
#include "stm32h7xx_hal_rcc_ex.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define FDCAN_1_NBR COMM_DEVICE_NUMBER_1
#define FDCAN_2_NBR COMM_DEVICE_NUMBER_2

typedef struct
{
    FDCAN_GlobalTypeDef *fdcan;
    FDCAN_HandleTypeDef hfdcan;
    FDCAN_RxHeaderTypeDef rxheader;
    uint64_t mostRecentInterrupTimestamp;
#if CAN_STATIC_TX_REPLAY_ENABLE
    uint32_t staticReplayBufferMask;
    uint32_t staticReplayBaseId;
    uint32_t staticReplayIdCount;
    uint32_t staticReplayNextIdOffset;
    uint32_t staticReplayLastCompletedMask;
    uint32_t staticReplayRequests;
    uint32_t staticReplayCompleted;
    uint32_t staticReplayIrqs;
    uint32_t staticReplayReloadErrors;
    uint8_t staticReplayPrepared;
    uint8_t staticReplayActive;
#endif
} FdcanInstanceType;

static FdcanInstanceType fdcan_hfdcan[FDCAN_MAX_INSTANCES] = {
    {.fdcan = NULL, .mostRecentInterrupTimestamp = 0},
    {.fdcan = NULL, .mostRecentInterrupTimestamp = 0}
};

CommDriverConfigType Can1Cfg;

/* Private function prototypes -----------------------------------------------*/
comm_status_t FDCAN_Ioctl(CommDriver *handle, int cmd, void *argument);

comm_status_t fdcan_find_free(FdcanInstanceType **handle)
{
    comm_status_t res = COMM_ERROR;

    for (uint8_t i = 0; i < FDCAN_MAX_INSTANCES; i++)
    {
        if (NULL == fdcan_hfdcan[i].fdcan)
        {
            *handle = &fdcan_hfdcan[i];
            res     = COMM_SUCCESS;
            break;
        }
    }

    return res;
}

comm_status_t fdcan_get_can(CommDriver *dev, FDCAN_HandleTypeDef **fdcan)
{
    FdcanInstanceType *instance;

    if (NULL == dev || NULL == dev->instance)
    {
        return COMM_NULL_POINTER;
    }

    instance = (FdcanInstanceType *)dev->instance;

    *fdcan = &instance->hfdcan;

    return COMM_SUCCESS;
}

comm_status_t
fdcan_get_handle(FDCAN_GlobalTypeDef *fdcan, FdcanInstanceType **handle)
{
    if (NULL == fdcan)
    {
        return COMM_ERROR;
    }

    for (uint8_t i = 0; i < FDCAN_MAX_INSTANCES; i++)
    {
        if (fdcan == fdcan_hfdcan[i].fdcan)
        {
            *handle = &fdcan_hfdcan[i];
            return COMM_SUCCESS;
        }
    }

    return COMM_ERROR;
}

comm_status_t get_fdcan_config(FdcanInstanceType *, FDCAN_FilterTypeDef *);

comm_status_t
fdcan_init_tx_header(const void *, FDCAN_TxHeaderTypeDef *, uint32_t);

#if CAN_STATIC_TX_REPLAY_ENABLE
static uint32_t fdcan_static_replay_buffer_mask(uint32_t buffer_count)
{
    if (buffer_count >= 32U)
    {
        return 0xFFFFFFFFU;
    }

    return (1UL << buffer_count) - 1UL;
}

static uint32_t fdcan_static_replay_base_id(FDCAN_GlobalTypeDef *fdcan)
{
    if (FDCAN_2 == fdcan)
    {
        return CAN_STATIC_TX_REPLAY_CAN2_BASE_ID;
    }

    return CAN_STATIC_TX_REPLAY_CAN1_BASE_ID;
}

static uint32_t fdcan_static_replay_channel_bit(FDCAN_GlobalTypeDef *fdcan)
{
    if (FDCAN_2 == fdcan)
    {
        return 0x2U;
    }

    return 0x1U;
}

static bool fdcan_static_replay_channel_enabled(FDCAN_GlobalTypeDef *fdcan)
{
    return (
        (CAN_STATIC_TX_REPLAY_CHANNEL_MASK
         & fdcan_static_replay_channel_bit(fdcan))
        != 0U
    );
}

static comm_status_t fdcan_static_replay_load_buffer(
    FdcanInstanceType *instance,
    uint32_t buffer_index,
    uint32_t id_offset
)
{
    FDCAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8] = {0U};

    if ((NULL == instance) || (0U == instance->staticReplayIdCount)
        || (buffer_index >= CAN_STATIC_TX_REPLAY_TX_BUFFERS))
    {
        return COMM_INVALID_PARAMETER;
    }

    memset(&tx_header, 0, sizeof(tx_header));
    tx_header.Identifier =
        (instance->staticReplayBaseId
         + (id_offset % instance->staticReplayIdCount))
        & 0x7FFU;
    tx_header.IdType              = FDCAN_STANDARD_ID;
    tx_header.TxFrameType         = FDCAN_DATA_FRAME;
    tx_header.DataLength          = FDCAN_DLC_BYTES_0;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch       = FDCAN_BRS_OFF;
    tx_header.FDFormat            = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker       = buffer_index & 0xFFU;

    if (HAL_OK
        != HAL_FDCAN_AddMessageToTxBuffer(
            &instance->hfdcan,
            &tx_header,
            tx_data,
            1UL << buffer_index
        ))
    {
        return COMM_ERROR;
    }

    return COMM_SUCCESS;
}

static comm_status_t fdcan_static_replay_prepare_instance(
    FdcanInstanceType *instance,
    uint32_t base_id,
    uint32_t id_count
)
{
    uint32_t buffer_count;
    uint32_t id_range;

    if (NULL == instance)
    {
        return COMM_NULL_POINTER;
    }

    buffer_count = CAN_STATIC_TX_REPLAY_TX_BUFFERS;
    if (buffer_count == 0U)
    {
        return COMM_INVALID_PARAMETER;
    }

    id_range = (id_count == 0U) ? 1U : id_count;
    if (id_range > 0x800U)
    {
        return COMM_INVALID_PARAMETER;
    }
    if (id_range < buffer_count)
    {
        return COMM_INVALID_PARAMETER;
    }

    instance->staticReplayPrepared         = 0U;
    instance->staticReplayActive           = 0U;
    instance->staticReplayBaseId           = base_id & 0x7FFU;
    instance->staticReplayIdCount          = id_range;
    instance->staticReplayNextIdOffset     = buffer_count % id_range;
    instance->staticReplayLastCompletedMask = 0U;
    instance->staticReplayReloadErrors     = 0U;
    instance->staticReplayBufferMask =
        fdcan_static_replay_buffer_mask(buffer_count);

    for (uint32_t i = 0U; i < buffer_count; i++)
    {
        if (COMM_SUCCESS != fdcan_static_replay_load_buffer(instance, i, i))
        {
            return COMM_ERROR;
        }
    }

    instance->staticReplayPrepared = 1U;

    return COMM_SUCCESS;
}

static comm_status_t
fdcan_static_replay_prepare_default(FdcanInstanceType *instance)
{
    if (!fdcan_static_replay_channel_enabled(instance->fdcan))
    {
        instance->staticReplayBufferMask        = 0U;
        instance->staticReplayLastCompletedMask = 0U;
        instance->staticReplayNextIdOffset      = 0U;
        instance->staticReplayPrepared          = 1U;
        instance->staticReplayActive            = 0U;
        return COMM_SUCCESS;
    }

    return fdcan_static_replay_prepare_instance(
        instance,
        fdcan_static_replay_base_id(instance->fdcan),
        CAN_STATIC_TX_REPLAY_ID_COUNT
    );
}

static void
fdcan_static_replay_rearm(FdcanInstanceType *instance, uint32_t mask)
{
    uint32_t request_mask;

    if ((NULL == instance) || (0U == instance->staticReplayActive)
        || (0U == instance->staticReplayPrepared))
    {
        return;
    }

    request_mask = mask & instance->staticReplayBufferMask;
    if (0U == request_mask)
    {
        return;
    }

    instance->hfdcan.Instance->TXBAR = request_mask;
    instance->staticReplayRequests += __builtin_popcount(request_mask);
}

static void fdcan_static_replay_rearm_completed(
    FdcanInstanceType *instance,
    uint32_t completed_mask
)
{
    uint32_t request_mask;

    if ((NULL == instance) || (0U == instance->staticReplayActive)
        || (0U == instance->staticReplayPrepared)
        || (0U == instance->staticReplayBufferMask))
    {
        return;
    }

    completed_mask &= instance->staticReplayBufferMask;
    instance->staticReplayLastCompletedMask = completed_mask;
    request_mask                            = 0U;

    for (uint32_t i = 0U; i < CAN_STATIC_TX_REPLAY_TX_BUFFERS; i++)
    {
        uint32_t buffer_bit = 1UL << i;

        if (0U == (completed_mask & buffer_bit))
        {
            continue;
        }

        if (COMM_SUCCESS
            == fdcan_static_replay_load_buffer(
                instance,
                i,
                instance->staticReplayNextIdOffset
            ))
        {
            request_mask |= buffer_bit;
            instance->staticReplayNextIdOffset++;
            if (instance->staticReplayNextIdOffset
                >= instance->staticReplayIdCount)
            {
                instance->staticReplayNextIdOffset = 0U;
            }
        }
        else
        {
            instance->staticReplayReloadErrors++;
        }
    }

    instance->staticReplayCompleted += __builtin_popcount(completed_mask);
    fdcan_static_replay_rearm(instance, request_mask);
}
#endif

comm_status_t FDCAN_GetMostRecentTimestamp(CommDriver *dev, uint64_t *timestamp)
{
    comm_status_t res = 0;
    FdcanInstanceType *instance;

    if (NULL == dev || NULL == dev->instance || NULL == timestamp)
    {
        res = COMM_NULL_POINTER;
        return res;
    }

    instance = (FdcanInstanceType *)dev->instance;

    if (COMM_SUCCESS != res)
    {
        FDCAN_ErrorHandler();
        return res;
    }

    *timestamp = instance->mostRecentInterrupTimestamp;
    return res;
}

uint64_t SampleTime(void);

comm_status_t FDCAN_CreateDriver(
    CommDriver *pDriver,
    const void *cfg,
    size_t cfg_size,
    uint8_t *tx,
    uint8_t *rx
)
{
    comm_status_t RetVal;
    FdcanInstanceType *instance;

    RetVal          = COMM_ERROR;
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
        RetVal          = COMM_SUCCESS;
        break;
    case COMM_DEVICE_NUMBER_2:
        instance->fdcan = FDCAN_2;
        RetVal          = COMM_SUCCESS;
        break;
    default:
        RetVal = COMM_ERROR;
        break;
    }

    if (COMM_SUCCESS != RetVal)
    {
        FDCAN_ErrorHandler();
    }

    pDriver->instance = (void *)instance;

    pDriver->interface->init  = FDCAN_Init;
    pDriver->interface->send  = FDCAN_Send;
    pDriver->interface->read  = FDCAN_Read;
    pDriver->interface->ioctl = FDCAN_Ioctl;
    pDriver->state            = DRIVER_STATE_UNINITIALIZED;
    pDriver->protocol         = DRIVER_FDCAN;
    pDriver->TxFrameBuffer    = tx;
    pDriver->RxFrameBuffer    = rx;

    pDriver->state = DRIVER_STATE_INITIALIZED;
    RetVal         = COMM_SUCCESS;

    return RetVal;
}

COMM_REGISTER_DRIVER(DRIVER_FDCAN, FDCAN_CreateDriver);

comm_status_t FDCAN_Init(CommDriver *dev)
{
    FDCAN_FilterTypeDef sFilterConfig;
    comm_status_t RetVal;
    FdcanInstanceType *instance;

    RetVal   = COMM_SUCCESS;
    instance = (FdcanInstanceType *)dev->instance;

    // TODO make init function consistent with driver creation
    get_fdcan_config(instance, &sFilterConfig);

#if FDCCAN_USE_TIMESTAMP_COUNTER
    HAL_FDCAN_EnableTimestampCounter(
        &instance->hfdcan,
        FDCAN_TIMESTAMP_EXTERNAL
    );
#else
    HAL_FDCAN_EnableTimestampCounter(
        &instance->hfdcan,
        FDCAN_TIMESTAMP_INTERNAL
    );
#endif

    if (HAL_FDCAN_Init(&instance->hfdcan) != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

#if CAN_STATIC_TX_REPLAY_ENABLE
    if (COMM_SUCCESS == RetVal
        && COMM_SUCCESS != fdcan_static_replay_prepare_default(instance))
    {
        RetVal = COMM_ERROR;
    }
#endif

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    if (HAL_FDCAN_ConfigTimestampCounter(
            &instance->hfdcan,
            FDCAN_TIMESTAMP_PRESC_1
        )
        != HAL_OK)
    {
        /* Initialization Error */
        RetVal = COMM_ERROR;
    }

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }

    if (HAL_FDCAN_EnableTimestampCounter(
            &instance->hfdcan,
            FDCAN_TIMESTAMP_EXTERNAL
        )
        != HAL_OK)
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

#if ((FDCAN_IRQ_NOTIFICATION & FDCAN_IT_RX_FIFO0_WATERMARK) != 0U)
#if FDCAN_IRQ_RX_WATERMARK < 2
#error "Implausible value for FDCAN_IRQ_RX_WATERMARK!"
#endif
    HAL_FDCAN_ConfigFifoWatermark(
        &instance->hfdcan,
        FDCAN_CFG_RX_FIFO0,
        FDCAN_IRQ_RX_WATERMARK
    );

    if (0 != instance->hfdcan.ErrorCode)
    {
        FDCAN_ErrorHandler();
    }
#endif

    if (HAL_FDCAN_ActivateNotification(
            &instance->hfdcan,
            FDCAN_IRQ_NOTIFICATION,
            0
        )
        != HAL_OK)
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

comm_status_t FDCAN_DeInit(CommDriver *dev)
{
    comm_status_t RetVal;
    FdcanInstanceType *instance;

    RetVal   = COMM_SUCCESS;
    instance = (FdcanInstanceType *)dev->instance;

    if (HAL_FDCAN_DeactivateNotification(
            &instance->hfdcan,
            FDCAN_IRQ_NOTIFICATION
        )
        != HAL_OK)
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

comm_status_t FDCAN_Send(CommDriver *dev, const void *pMsg)
{
#if CAN_STATIC_TX_REPLAY_ENABLE
    (void)dev;
    (void)pMsg;
    return COMM_INVALID_STATE;
#else
    comm_status_t RetVal;
    uint8_t *pData;
    FDCAN_Message *pMsgCpy;
    FDCAN_TxHeaderTypeDef TxHeader;
    FdcanInstanceType *instance;

    if (DRIVER_STATE_STARTED != dev->state)
    {
        return COMM_INVALID_STATE;
    }

    RetVal   = COMM_SUCCESS;
    instance = (FdcanInstanceType *)dev->instance;
    pMsgCpy  = (FDCAN_Message *)pMsg;
    pData    = (uint8_t *)(pMsgCpy->msgBase.payload);

    (void)fdcan_init_tx_header(pMsgCpy, &TxHeader, pMsgCpy->msgBase.length);

    if (HAL_FDCAN_STATE_BUSY != HAL_FDCAN_GetState(&instance->hfdcan))
    {
        RetVal = COMM_ERROR;
    }
    else if (HAL_FDCAN_AddMessageToTxFifoQ(&instance->hfdcan, &TxHeader, pData)
             == HAL_OK)
    {
        RetVal = COMM_SUCCESS;
    }
    else
    {
        RetVal = COMM_ERROR;
    }

    return RetVal;
#endif
}

comm_status_t
FDCAN_Read(CommDriver *dev, void *pFrame, uint8_t length, uint32_t RxFifo0ITs)
{
    comm_status_t RetVal;
    uint8_t Data[64];
    FDCAN_ClassicFrameType *pNewFrame;
    FdcanInstanceType *instance;
    FDCAN_RxHeaderTypeDef rxheader;
    uint8_t PayloadLen = 0;
    uint32_t Brs;
    uint32_t Esi;
    uint32_t Ide;
    uint32_t FdFormat;

    RetVal    = COMM_SUCCESS;
    instance  = (FdcanInstanceType *)dev->instance;
    pNewFrame = (FDCAN_ClassicFrameType *)pFrame;

    if ((RxFifo0ITs & FDCAN_IRQ_NOTIFICATION) != RESET)
    {
        memset(&rxheader, 0, sizeof(rxheader));

        /* Retreive Rx messages from RX FIFO0 */
        if (HAL_FDCAN_GetRxMessage(
                &instance->hfdcan,
                FDCAN_RX_FIFO0,
                &rxheader,
                Data
            )
            != HAL_OK)
        {
            /* Reception Error */
            RetVal = COMM_ERROR;
        }

        if (RetVal != COMM_SUCCESS)
        {
            return RetVal;
        }

        pNewFrame->dlc_dl_flags = 0U;

        Brs = (rxheader.BitRateSwitch & FDCAN_BRS_ON) ? 1U : 0U;
        FDCAN_SET_BRS(pNewFrame, Brs);

        Esi = (rxheader.ErrorStateIndicator & FDCAN_ESI_PASSIVE) ? 1U : 0U;
        FDCAN_SET_ESI(pNewFrame, Esi);

        Ide = (rxheader.IdType & FDCAN_EXTENDED_ID) ? 1U : 0U;
        FDCAN_SET_IDE(pNewFrame, Ide);

        FdFormat = (rxheader.FDFormat & FDCAN_FD_CAN) ? 1U : 0U;
        FDCAN_SET_FDF(pNewFrame, FdFormat);

        switch (rxheader.DataLength)
        {
        case FDCAN_DLC_BYTES_0:
            FDCAN_SET_DLC(pNewFrame, 0);
            FDCAN_SET_DATA_LEN(pNewFrame, 0);
            break;
        case FDCAN_DLC_BYTES_1:
            FDCAN_SET_DLC(pNewFrame, 1);
            FDCAN_SET_DATA_LEN(pNewFrame, 1);
            break;
        case FDCAN_DLC_BYTES_2:
            FDCAN_SET_DLC(pNewFrame, 2);
            FDCAN_SET_DATA_LEN(pNewFrame, 2);
            break;
        case FDCAN_DLC_BYTES_3:
            FDCAN_SET_DLC(pNewFrame, 3);
            FDCAN_SET_DATA_LEN(pNewFrame, 3);
            break;
        case FDCAN_DLC_BYTES_4:
            FDCAN_SET_DLC(pNewFrame, 4);
            FDCAN_SET_DATA_LEN(pNewFrame, 4);
            break;
        case FDCAN_DLC_BYTES_5:
            FDCAN_SET_DLC(pNewFrame, 5);
            FDCAN_SET_DATA_LEN(pNewFrame, 5);
            break;
        case FDCAN_DLC_BYTES_6:
            FDCAN_SET_DLC(pNewFrame, 6);
            FDCAN_SET_DATA_LEN(pNewFrame, 6);
            break;
        case FDCAN_DLC_BYTES_7:
            FDCAN_SET_DLC(pNewFrame, 7);
            FDCAN_SET_DATA_LEN(pNewFrame, 7);
            break;
        case FDCAN_DLC_BYTES_8:
            FDCAN_SET_DLC(pNewFrame, 8);
            FDCAN_SET_DATA_LEN(pNewFrame, 8);
            break;
        case FDCAN_DLC_BYTES_12:
            FDCAN_SET_DLC(pNewFrame, 9);
            FDCAN_SET_DATA_LEN(pNewFrame, 12);
            break;
        case FDCAN_DLC_BYTES_16:
            FDCAN_SET_DLC(pNewFrame, 10);
            FDCAN_SET_DATA_LEN(pNewFrame, 16);
            break;
        case FDCAN_DLC_BYTES_20:
            FDCAN_SET_DLC(pNewFrame, 11);
            FDCAN_SET_DATA_LEN(pNewFrame, 20);
            break;
        case FDCAN_DLC_BYTES_24:
            FDCAN_SET_DLC(pNewFrame, 12);
            FDCAN_SET_DATA_LEN(pNewFrame, 24);
            break;
        case FDCAN_DLC_BYTES_32:
            FDCAN_SET_DLC(pNewFrame, 13);
            FDCAN_SET_DATA_LEN(pNewFrame, 32);
            break;
        case FDCAN_DLC_BYTES_48:
            FDCAN_SET_DLC(pNewFrame, 14);
            FDCAN_SET_DATA_LEN(pNewFrame, 48);
            break;
        case FDCAN_DLC_BYTES_64:
            FDCAN_SET_DLC(pNewFrame, 15);
            FDCAN_SET_DATA_LEN(pNewFrame, 64);
            break;
        default:
            FDCAN_SET_DLC(pNewFrame, 0);
            FDCAN_SET_DATA_LEN(pNewFrame, 0);
        };

        pNewFrame->id        = rxheader.Identifier;
        pNewFrame->timestamp = rxheader.RxTimestamp;

        PayloadLen = FDCAN_GET_DATA_LEN(pNewFrame);
        if (PayloadLen > sizeof(Data))
        {
            PayloadLen = sizeof(Data);
        }
        memcpy(pNewFrame->data, &Data[0], PayloadLen);

        if (HAL_FDCAN_ActivateNotification(
                &instance->hfdcan,
                FDCAN_IRQ_NOTIFICATION,
                0
            )
            != HAL_OK)
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

uint64_t FDCAN_GetMostRecentInterruptTimestamp(CommDriver *dev)
{
    return ((FdcanInstanceType *)dev->instance)->mostRecentInterrupTimestamp;
}

#if CAN_STATIC_TX_REPLAY_ENABLE
comm_status_t FDCAN_StaticTxReplayPrepare(
    CommDriver *dev,
    uint32_t base_id,
    uint32_t id_count
)
{
    FdcanInstanceType *instance;

    if ((NULL == dev) || (NULL == dev->instance))
    {
        return COMM_NULL_POINTER;
    }

    instance = (FdcanInstanceType *)dev->instance;
    if (!fdcan_static_replay_channel_enabled(instance->hfdcan.Instance))
    {
        instance->staticReplayBufferMask        = 0U;
        instance->staticReplayLastCompletedMask = 0U;
        instance->staticReplayNextIdOffset      = 0U;
        instance->staticReplayPrepared          = 1U;
        instance->staticReplayActive            = 0U;
        return COMM_SUCCESS;
    }

    return fdcan_static_replay_prepare_instance(instance, base_id, id_count);
}

comm_status_t FDCAN_StaticTxReplayStart(CommDriver *dev)
{
    FdcanInstanceType *instance;

    if ((NULL == dev) || (NULL == dev->instance))
    {
        return COMM_NULL_POINTER;
    }

    instance = (FdcanInstanceType *)dev->instance;
    if (!fdcan_static_replay_channel_enabled(instance->hfdcan.Instance))
    {
        instance->staticReplayActive = 0U;
        return COMM_SUCCESS;
    }

    if (0U == instance->staticReplayPrepared)
    {
        return COMM_INVALID_STATE;
    }

    instance->staticReplayActive = 1U;
    instance->staticReplayLastCompletedMask = 0U;
    fdcan_static_replay_rearm(instance, instance->staticReplayBufferMask);

    return COMM_SUCCESS;
}

comm_status_t FDCAN_StaticTxReplayStop(CommDriver *dev)
{
    FdcanInstanceType *instance;

    if ((NULL == dev) || (NULL == dev->instance))
    {
        return COMM_NULL_POINTER;
    }

    instance                                = (FdcanInstanceType *)dev->instance;
    instance->staticReplayActive            = 0U;
    instance->staticReplayLastCompletedMask = 0U;

    if (!fdcan_static_replay_channel_enabled(instance->hfdcan.Instance))
    {
        return COMM_SUCCESS;
    }

    if (HAL_OK
        != HAL_FDCAN_DeactivateNotification(
            &instance->hfdcan,
            FDCAN_IT_TX_COMPLETE
        ))
    {
        return COMM_ERROR;
    }

    __HAL_FDCAN_CLEAR_IT(&instance->hfdcan, FDCAN_IT_TX_COMPLETE);

    return COMM_SUCCESS;
}

comm_status_t FDCAN_StaticTxReplayGetStats(
    CommDriver *dev,
    FdcanStaticTxReplayStatsType *stats
)
{
    FdcanInstanceType *instance;

    if ((NULL == dev) || (NULL == dev->instance) || (NULL == stats))
    {
        return COMM_NULL_POINTER;
    }

    instance = (FdcanInstanceType *)dev->instance;

    stats->requests            = instance->staticReplayRequests;
    stats->completed           = instance->staticReplayCompleted;
    stats->irqs                = instance->staticReplayIrqs;
    stats->last_completed_mask = instance->staticReplayLastCompletedMask;
    stats->buffer_mask         = instance->staticReplayBufferMask;
    stats->next_id_offset      = instance->staticReplayNextIdOffset;
    stats->tx_pending =
        instance->hfdcan.Instance->TXBRP & instance->staticReplayBufferMask;
    stats->tx_occurred =
        instance->hfdcan.Instance->TXBTO & instance->staticReplayBufferMask;
    stats->tx_cancelled =
        instance->hfdcan.Instance->TXBCF & instance->staticReplayBufferMask;
    stats->protocol_status = instance->hfdcan.Instance->PSR;
    stats->error_counter   = instance->hfdcan.Instance->ECR;
    stats->reload_errors   = instance->staticReplayReloadErrors;
    stats->active          = instance->staticReplayActive;

    return COMM_SUCCESS;
}

void FDCAN_StaticTxReplayIrqHandler(FDCAN_GlobalTypeDef *fdcan)
{
    FdcanInstanceType *instance;
    uint32_t completed;

    if (COMM_SUCCESS != fdcan_get_handle(fdcan, &instance))
    {
        FDCAN_ErrorHandler();
        return;
    }

    if (!fdcan_static_replay_channel_enabled(instance->hfdcan.Instance))
    {
        return;
    }

    completed = instance->hfdcan.Instance->TXBTO;
    completed &= instance->staticReplayBufferMask;

    __HAL_FDCAN_CLEAR_FLAG(&instance->hfdcan, FDCAN_FLAG_TX_COMPLETE);

    if (0U != completed)
    {
        instance->staticReplayIrqs++;
        fdcan_static_replay_rearm_completed(instance, completed);
    }
}
#endif

void FDCAN_1_IRQHandler(void)
{
    FdcanInstanceType *instance;

    if (COMM_SUCCESS != fdcan_get_handle(FDCAN1, &instance))
    {
        FDCAN_ErrorHandler();
    }

    /* capture timestamp at the moment of the interrupt */
    instance->mostRecentInterrupTimestamp = SampleTime();
    HAL_FDCAN_IRQHandler(&instance->hfdcan);
}

void FDCAN_2_IRQHandler(void)
{
    FdcanInstanceType *instance;

    if (COMM_SUCCESS != fdcan_get_handle(FDCAN2, &instance))
    {
        FDCAN_ErrorHandler();
    }

    instance->mostRecentInterrupTimestamp = SampleTime();
    HAL_FDCAN_IRQHandler(&instance->hfdcan);
}

#if CAN_STATIC_TX_REPLAY_ENABLE
void FDCAN_1_TX_IRQHandler(void)
{
    FDCAN_StaticTxReplayIrqHandler(FDCAN_1);
}

void FDCAN_2_TX_IRQHandler(void)
{
    FDCAN_StaticTxReplayIrqHandler(FDCAN_2);
}
#endif

/* IOCTL/ driver specific functions */

static comm_status_t FDCAN_SetBaudrate(CommDriver *dev, uint32_t baudrate)
{
    comm_status_t res   = 0;
    uint32_t FdcanClock = 0;
    uint8_t timings[4];
    bool IsDataPhase = false;
    uint16_t Prescaler;
    uint8_t Seg1;
    uint8_t Seg2;
    uint8_t Sjw;
    FdcanInstanceType *instance;

    instance   = (FdcanInstanceType *)dev->instance;
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
    else if (0
             == CANFD_CalculateBitTimingRegister(
                 FdcanClock,
                 baudrate,
                 7500,
                 1,
                 IsDataPhase,
                 timings
             ))
    {
        res = COMM_SUCCESS;
    }
    else
    {
        res = COMM_ERROR;
    }

    if (COMM_SUCCESS == res)
    {
        Prescaler = CANFD_GetPrescaler(timings, IsDataPhase);
        Seg1      = CANFD_GetSeg1(timings, IsDataPhase);
        Seg2      = CANFD_GetSeg2(timings, IsDataPhase);
        Sjw       = CANFD_GetSJW(timings, IsDataPhase);

        instance->hfdcan.Init.NominalPrescaler     = Prescaler;
        instance->hfdcan.Init.NominalSyncJumpWidth = Sjw;
        instance->hfdcan.Init.NominalTimeSeg1      = Seg1;
        instance->hfdcan.Init.NominalTimeSeg2      = Seg2;

        if (HAL_FDCAN_Init(&instance->hfdcan) != HAL_OK)
        {
            /* Initialization Error */
            res = COMM_ERROR;
        }

#if CAN_STATIC_TX_REPLAY_ENABLE
        if (COMM_SUCCESS == res
            && COMM_SUCCESS != fdcan_static_replay_prepare_default(instance))
        {
            res = COMM_ERROR;
        }
#endif
    }

    return res;
}

static comm_status_t FDCAN_SetMode(CommDriver *dev, uint32_t mode)
{
    comm_status_t res = 0;
    FdcanInstanceType *instance;

    instance = (FdcanInstanceType *)dev->instance;

    switch (mode)
    {
    case FDCAN_MODE_1:
#if CAN_STATIC_TX_REPLAY_ENABLE
        instance->hfdcan.Init.Mode = CAN_STATIC_TX_REPLAY_FDCAN_MODE;
#else
        instance->hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
#endif
        res = COMM_SUCCESS;
        break;
    case FDCAN_MODE_2:
        instance->hfdcan.Init.Mode = FDCAN_MODE_BUS_MONITORING;
        res                        = COMM_SUCCESS;
        break;
    case FDCAN_MODE_4:
        instance->hfdcan.Init.Mode = FDCAN_MODE_EXTERNAL_LOOPBACK;
        res                        = COMM_SUCCESS;
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

#if CAN_STATIC_TX_REPLAY_ENABLE
        if (COMM_SUCCESS == res
            && COMM_SUCCESS != fdcan_static_replay_prepare_default(instance))
        {
            res = COMM_ERROR;
        }
#endif
    }

    return res;
}

comm_status_t FDCAN_Ioctl(CommDriver *dev, int cmd, void *argument)
{
    int HalRes        = 0;
    comm_status_t res = 0;
    FdcanInstanceType *instance;

    instance = (FdcanInstanceType *)dev->instance;

    switch (cmd)
    {
    case CANABS_IOCTL_CMD_SET_BAUDRATE: {
        FdcanBaudrateType baudrate = *((FdcanBaudrateType *)argument);
        res                        = FDCAN_SetBaudrate(dev, baudrate);
        if (COMM_SUCCESS == res)
        {
            dev->state = DRIVER_STATE_INITIALIZED;
        }
    }
    break;
    case CANABS_IOCTL_CMD_START:
        if (DRIVER_STATE_STARTED == dev->state)
        {
            /* nothing to do */
        }
        else if (HAL_OK != HAL_FDCAN_Start(&instance->hfdcan))
        {
            /* Start Error */
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
#if CAN_STATIC_TX_REPLAY_ENABLE
        else if (!fdcan_static_replay_channel_enabled(instance->hfdcan.Instance
                 ))
        {
            if (HAL_OK
                != HAL_FDCAN_ActivateNotification(
                    &instance->hfdcan,
                    FDCAN_IRQ_NOTIFICATION,
                    0
                ))
            {
                res = COMM_ERROR;
                FDCAN_ErrorHandler();
            }
        }
        else if (0U == instance->staticReplayPrepared)
        {
            res = COMM_INVALID_STATE;
            FDCAN_ErrorHandler();
        }
        else if (HAL_OK
                 != HAL_FDCAN_ConfigInterruptLines(
                     &instance->hfdcan,
                     FDCAN_IT_TX_COMPLETE,
                     FDCAN_INTERRUPT_LINE1
                 ))
        {
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
        else if (HAL_OK
                 != HAL_FDCAN_ActivateNotification(
                     &instance->hfdcan,
                     FDCAN_IRQ_NOTIFICATION | FDCAN_IT_TX_COMPLETE,
                     instance->staticReplayBufferMask
                 ))
        {
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
        else
        {
            instance->staticReplayActive            = 1U;
            instance->staticReplayLastCompletedMask = 0U;
            fdcan_static_replay_rearm(
                instance,
                instance->staticReplayBufferMask
            );
        }
#else
        else if (HAL_OK
                 == HAL_FDCAN_AbortTxRequest(
                     &instance->hfdcan,
                     FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2
                 ))
        {
            HAL_FDCAN_ActivateNotification(
                &instance->hfdcan,
                FDCAN_IRQ_NOTIFICATION,
                0
            );
        }
#endif

        if (COMM_SUCCESS == res)
        {
            dev->state = DRIVER_STATE_STARTED;
        }
        else
        {
            res = COMM_ERROR;
        }
        break;
    case CANABS_IOCTL_CMD_STOP:
        if (DRIVER_STATE_STARTED != dev->state)
        {
            /* nothing to do */
        }
#if CAN_STATIC_TX_REPLAY_ENABLE
        else if (COMM_SUCCESS != FDCAN_StaticTxReplayStop(dev))
        {
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
#endif
        else if ((HAL_FDCAN_DeactivateNotification(
                      &instance->hfdcan,
                      FDCAN_IRQ_NOTIFICATION
                  )
                  != HAL_OK))
        {
            /* Start Error */
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
        else if (HAL_FDCAN_Stop(&instance->hfdcan) != HAL_OK)
        {
            /* Start Error */
            res = COMM_ERROR;
            FDCAN_ErrorHandler();
        }
        else if ((HalRes = HAL_FDCAN_AbortTxRequest(
                      &instance->hfdcan,
#if CAN_STATIC_TX_REPLAY_ENABLE
                      instance->staticReplayBufferMask
#else
                      FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2
#endif
                  ))
                 != HAL_OK)
        {
            /* Start Error */
            res = COMM_ERROR;
            (void)(HalRes);
            FDCAN_ErrorHandler();
        }
        else
        {
            __HAL_FDCAN_CLEAR_IT(&instance->hfdcan, FDCAN_IRQ_NOTIFICATION);
        }

        if (COMM_SUCCESS == res
            || (instance->hfdcan.ErrorCode & HAL_FDCAN_ERROR_NOT_STARTED))
        {
            dev->state = DRIVER_STATE_STOPPED;
        }
        break;
    case CANABS_IOCTL_CMD_SET_MODE: {
        FdcanModeType mode = *((FdcanModeType *)argument);
        res                = FDCAN_SetMode(dev, mode);

        if (COMM_SUCCESS == res && mode == FDCAN_MODE_3)
        {
            dev->state = DRIVER_STATE_OFF;
        }
        else if (COMM_SUCCESS == res)
        {
            dev->state = DRIVER_STATE_INITIALIZED;
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

    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    if (FDCAN_1 == fdcan)
    {
        GPIO_InitStruct.Pin       = FDCAN_1_TX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_1_TX_AF;
        (void)HAL_GPIO_Init(FDCAN_1_TX_GPIO_PORT, &GPIO_InitStruct);

        /* FDCANx RX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = FDCAN_1_RX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_1_RX_AF;
        (void)HAL_GPIO_Init(FDCAN_1_RX_GPIO_PORT, &GPIO_InitStruct);
    }
    else if (FDCAN_2 == fdcan)
    {
        GPIO_InitStruct.Pin       = FDCAN_2_TX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_2_TX_AF;
        (void)HAL_GPIO_Init(FDCAN_2_TX_GPIO_PORT, &GPIO_InitStruct);

        /* FDCANx RX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = FDCAN_2_RX_PIN;
        GPIO_InitStruct.Alternate = FDCAN_2_RX_AF;
        (void)HAL_GPIO_Init(FDCAN_2_RX_GPIO_PORT, &GPIO_InitStruct);
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
#if CAN_STATIC_TX_REPLAY_ENABLE
        HAL_NVIC_SetPriority(
            FDCAN_1_TX_IRQn,
            FDCAN_TX_REPLAY_IRQ_PREEMPT_PRIO,
            1
        );
        HAL_NVIC_EnableIRQ(FDCAN_1_TX_IRQn);
#endif
    }
    else if (FDCAN_2 == fdcan)
    {
        HAL_NVIC_SetPriority(FDCAN_2_IRQn, FDCAN_IRQ_PREEMPT_PRIO, 1);
        HAL_NVIC_EnableIRQ(FDCAN_2_IRQn);
#if CAN_STATIC_TX_REPLAY_ENABLE
        HAL_NVIC_SetPriority(
            FDCAN_2_TX_IRQn,
            FDCAN_TX_REPLAY_IRQ_PREEMPT_PRIO,
            1
        );
        HAL_NVIC_EnableIRQ(FDCAN_2_TX_IRQn);
#endif
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
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan)
{
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    (void)FDCAN_GpioClck(hfdcan->Instance);

    /* Select PLL1Q as source of FDCANx clock */
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    RCC_PeriphClkInit.FdcanClockSelection  = RCC_FDCANCLKSOURCE_PLL;
    (void)HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    /* Enable FDCANx clock */
    FDCANx_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* FDCANx TX GPIO pin configuration  */
    (void)FDCAN_InitGpio(hfdcan->Instance);

    /*##-3- Configure the NVIC #################################################*/
    /* NVIC for FDCANx */
    (void)FDCAN_Nvic(hfdcan->Instance);
}

/**
  * @brief  DeInitializes the FDCAN MSP.
  * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @retval None
  */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *hfdcan)
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
#if CAN_STATIC_TX_REPLAY_ENABLE
        HAL_NVIC_DisableIRQ(FDCAN_1_TX_IRQn);
#endif
    }
    else if (FDCAN_2 == hfdcan->Instance)
    {
        HAL_GPIO_DeInit(FDCAN_2_TX_GPIO_PORT, FDCAN_2_TX_PIN);
        HAL_GPIO_DeInit(FDCAN_2_RX_GPIO_PORT, FDCAN_2_RX_PIN);
        HAL_NVIC_DisableIRQ(FDCAN_2_IRQn);
#if CAN_STATIC_TX_REPLAY_ENABLE
        HAL_NVIC_DisableIRQ(FDCAN_2_TX_IRQn);
#endif
    }
    else
    {
        FDCAN_ErrorHandler();
    }
}

comm_status_t fdcan_init_tx_header(
    const void *pMsg,
    FDCAN_TxHeaderTypeDef *pTxHeader,
    uint32_t frameLength
)
{
    comm_status_t RetVal;
    FDCAN_Message *pMsgCopy;

    RetVal   = COMM_ERROR;
    pMsgCopy = (FDCAN_Message *)pMsg;

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
    pTxHeader->BitRateSwitch       = FDCAN_BRS_OFF;
    pTxHeader->FDFormat            = FDCAN_CLASSIC_CAN;
    pTxHeader->TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    pTxHeader->MessageMarker       = pMsgCopy->msgMarker;

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
    FdcanInstanceType *instance,
    FDCAN_FilterTypeDef *pFilterConfig
)
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
    instance->hfdcan.Instance         = instance->fdcan;
    instance->hfdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
#if CAN_STATIC_TX_REPLAY_ENABLE
    instance->hfdcan.Init.Mode = CAN_STATIC_TX_REPLAY_FDCAN_MODE;
#else
    instance->hfdcan.Init.Mode = FDCAN_MODE_DEFAULT;
#endif
    instance->hfdcan.Init.AutoRetransmission = ENABLE;
    instance->hfdcan.Init.TransmitPause      = DISABLE;
    instance->hfdcan.Init.ProtocolException  = ENABLE;
    instance->hfdcan.Init.NominalPrescaler =
        0x4; /* tq = NominalPrescaler x (1/fdcan_ker_ck) */
    instance->hfdcan.Init.NominalSyncJumpWidth = 0x01;
    instance->hfdcan.Init.NominalTimeSeg1 =
        34U; /* NominalTimeSeg1 = Propagation_segment + Phase_segment_1 */
    instance->hfdcan.Init.NominalTimeSeg2 = 5U;
    ram_usage(instance->fdcan, &instance->hfdcan.Init.MessageRAMOffset);
    instance->hfdcan.Init.StdFiltersNbr   = 1;
    instance->hfdcan.Init.ExtFiltersNbr   = 0;
    instance->hfdcan.Init.RxFifo0ElmtsNbr = FDCAN_RAM_RX_ELEMENTS;
    instance->hfdcan.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    instance->hfdcan.Init.RxFifo1ElmtsNbr = 0;
    instance->hfdcan.Init.RxBuffersNbr    = 0;
    instance->hfdcan.Init.TxEventsNbr     = 0;
#if CAN_STATIC_TX_REPLAY_ENABLE
    if (fdcan_static_replay_channel_enabled(instance->fdcan))
    {
        instance->hfdcan.Init.TxBuffersNbr = CAN_STATIC_TX_REPLAY_TX_BUFFERS;
        instance->hfdcan.Init.TxFifoQueueElmtsNbr = 0;
    }
    else
    {
        instance->hfdcan.Init.TxBuffersNbr        = 0;
        instance->hfdcan.Init.TxFifoQueueElmtsNbr = FDCAN_RAM_TX_ELEMENTS;
    }
#else
    instance->hfdcan.Init.TxBuffersNbr        = 0;
    instance->hfdcan.Init.TxFifoQueueElmtsNbr = FDCAN_RAM_TX_ELEMENTS;
#endif
    instance->hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    instance->hfdcan.Init.TxElmtSize      = FDCAN_DATA_BYTES_8;

    pFilterConfig->IdType       = FDCAN_STANDARD_ID;
    pFilterConfig->FilterIndex  = 0;
    pFilterConfig->FilterType   = FDCAN_FILTER_MASK;
    pFilterConfig->FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    pFilterConfig->FilterID1    = 0x0;
    pFilterConfig->FilterID2    = 0x7FF;

    RetVal = COMM_SUCCESS;

    return RetVal;
}

uint64_t SampleTime(void)
{
    return FDCAN_GetTimestampHook();
}
