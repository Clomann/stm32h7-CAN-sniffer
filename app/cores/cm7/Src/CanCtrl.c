#include "CanCtrl.h"

#include "CommTypes.h"
#include "timer.h"
#include "CanAbs.h"
#include "fdcan.h"
#include <stdint.h>

#define TIMx_TIME_RESOLUTION (1U)

/*!< Time in micro seconds */
static volatile uint64_t Time = 0;

static FDCAN_Message Can1TestMsg1;
static FDCAN_Message Can1TestMsg2;
static FDCAN_Message Can2TestMsg1;
static FDCAN_Message Can2TestMsg2;

uint64_t CANABS_ConvertCountToTimestampHook(uint32_t cnt)
{
    return (uint64_t)TIMx_TIME_RESOLUTION * (uint64_t)cnt;
}

void appFdcanInit(CanCtrlDataType *data)
{
    static uint8_t TxData[8];
    static uint8_t TxData2[8];

    memset(TxData, 0xFF, sizeof(TxData));
    memset(TxData2, 0xFF, sizeof(TxData2));

    /* Initialize FDCAN timestamp external timer */
    if (0 != TIMx_Init(TIMx_TIME_RESOLUTION))
    {
        CANCONTROL_ErrorHandlerHook();
    }

    if (0 == CanAbs_Init_Can1(data->can1.baudrate))
    {
        CanAbs_CreateMessage_Standard(
            &Can1TestMsg1,
            0x321,
            &TxData[0],
            sizeof(TxData) / sizeof(*TxData)
        );
        CanAbs_CreateMessage_Standard(
            &Can1TestMsg2,
            0x322,
            &TxData[0],
            sizeof(TxData) / sizeof(*TxData)
        );
    }
    else
    {
        CANCONTROL_ErrorHandlerHook();
    }

    if (0 == CanAbs_Init_Can2(data->can2.baudrate))
    {
        CanAbs_CreateMessage_Standard(
            &Can2TestMsg1,
            0x323,
            &TxData2[0],
            sizeof(TxData2) / sizeof(*TxData2)
        );
        CanAbs_CreateMessage_Standard(
            &Can2TestMsg2,
            0x324,
            &TxData2[0],
            sizeof(TxData2) / sizeof(*TxData2)
        );
    }
    else
    {
        CANCONTROL_ErrorHandlerHook();
    }
}

void appFdcanPoll(_Bool sendingActive)
{
    volatile uint8_t res;

    if (sendingActive)
    {
        res = CanAbs_Send_Can1(&Can1TestMsg1);
        if (0 != res)
        {
            CANCONTROL_ErrorHandlerHook();
        }

        res = CanAbs_Send_Can1(&Can1TestMsg2);
        if (0 != res)
        {
            CANCONTROL_ErrorHandlerHook();
        }

        res = CanAbs_Send_Can2(&Can2TestMsg1);
        if (0 != res)
        {
            CANCONTROL_ErrorHandlerHook();
        }

        res = CanAbs_Send_Can2(&Can2TestMsg2);
        if (0 != res)
        {
            CANCONTROL_ErrorHandlerHook();
        }
    }
}

void appCanCtrlSetBaudrate(CanCtrlDataType *data)
{
    if (COMM_SUCCESS != CanAbs_SetBaudrate_Can1(data->can1.baudrate))
    {
        CANCONTROL_ErrorHandlerHook();
    }

    if (COMM_SUCCESS != CanAbs_SetBaudrate_Can2(data->can2.baudrate))
    {
        CANCONTROL_ErrorHandlerHook();
    }
}

void appCanCtrlSetMode(CanCtrlDataType *data)
{
    if (COMM_SUCCESS != CanAbs_SetMode_Can1(data->can1.mode))
    {
        CANCONTROL_ErrorHandlerHook();
    }

    if (COMM_SUCCESS != CanAbs_SetMode_Can2(data->can2.mode))
    {
        CANCONTROL_ErrorHandlerHook();
    }
}

void TIM_InterruptCallback(void)
{
    uint64_t Period = 0U;

    Period = FDCAN_GetTimerPeriodHook();

    /* add one full timer period to timer */
    Time += Period;

    CANABS_CheckIsrPollPeriod(Time, Period);
}

void TIM_HAL_InterruptCallback()
{
    HAL_IncTick();
}

/**
 * 
 * \param[out] timestamp in micro seconds.
 */
uint64_t FDCAN_GetTimestampHook(void)
{
    uint64_t timestamp;
    uint64_t time_snapshot1, time_snapshot2;
    uint32_t cnt;
    uint32_t uif;

    do
    {
        time_snapshot1 = Time;
        TIM_GetCounterValueAndUpdateInterruptFlag(&cnt, &uif);
        time_snapshot2 = Time;
    } while (time_snapshot1 != time_snapshot2);

    timestamp = time_snapshot1 + (uint64_t)(cnt * TIMx_TIME_RESOLUTION);

    if (uif)
    {
        timestamp += FDCAN_GetTimerPeriodHook();
    }

    return timestamp;
}

uint64_t FDCAN_GetTimerPeriodHook(void)
{
    uint16_t Arr;

    TIM_GetArrValue(&Arr);

    return ((uint64_t)Arr + (uint64_t)1U) * (uint64_t)TIMx_TIME_RESOLUTION;
}
