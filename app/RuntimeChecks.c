#include "RuntimeChecks.h"
#include "cmsis_gcc.h"

volatile uint64_t CanAbs_FrameCount = 0;
volatile uint64_t CanAbs_FrameDropCount = 0;
volatile uint64_t CanAbs_CAN1_Rx_FrameDropCount = 0;
volatile uint64_t CanAbs_CAN2_Rx_FrameDropCount = 0;
volatile uint64_t CanBridgeTask_FrameCount = 0;
volatile uint64_t CanLogManager_FrameCount = 0;
volatile uint64_t CanLogManager_FrameDropCount1 = 0;
volatile uint64_t CanLogManager_FrameDropCount2 = 0;
volatile uint64_t CanLogBuffer_FrameCount1 = 0;
volatile uint64_t CanLogBuffer_FrameCount2 = 0;
volatile uint64_t CanLogBuffer_BlockCount = 0;
volatile uint64_t FcdanMsgPort_FrameDropCount = 0;
volatile uint64_t FrameDelta1 = 0;
volatile uint64_t FrameDelta2 = 0;
volatile uint64_t ErrorHandlerCalls = 0;

static void RuntimeChecks_ErrorHandler(void)
{
    ErrorHandlerCalls++;
}

void RuntimeChecks_Init(void)
{
    CanAbs_FrameCount = 0;
    CanAbs_FrameDropCount = 0;
    CanAbs_CAN1_Rx_FrameDropCount = 0;
    CanAbs_CAN2_Rx_FrameDropCount = 0;
    CanBridgeTask_FrameCount = 0;
    CanLogManager_FrameCount = 0;
    CanLogManager_FrameDropCount1 = 0;
    CanLogManager_FrameDropCount2 = 0;
    CanLogBuffer_FrameCount1 = 0;
    CanLogBuffer_FrameCount2 = 0;
    CanLogBuffer_BlockCount = 0;
    FcdanMsgPort_FrameDropCount = 0;
    ErrorHandlerCalls = 0;
}

static RuntimeChecksErrorType CheckDropCounts(void)
{
    RuntimeChecksErrorType err = RUNTIMECHECKS_E_OK;

    FrameDelta1 = CanBridgeTask_FrameCount - CanLogManager_FrameCount;
    FrameDelta2 = CanLogBuffer_FrameCount1 - CanLogBuffer_FrameCount2;

    if (0 != FcdanMsgPort_FrameDropCount)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanAbs_FrameDropCount) 
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanAbs_CAN1_Rx_FrameDropCount) 
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanAbs_CAN2_Rx_FrameDropCount) 
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (FrameDelta1 > 1024)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (FrameDelta2 > 1024)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanLogManager_FrameDropCount1)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanLogManager_FrameDropCount2)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }

    return err;
}

void RuntimeChecks_CheckFrameCounts(RuntimeChecksContextType *context)
{
    RuntimeChecksErrorType err;

    
    err = CheckDropCounts();
    
    if (RUNTIMECHECKS_E_OK != err)
    {
        (void) context;
        RuntimeChecks_ErrorHandler();
    }

    context->err = err;
}
