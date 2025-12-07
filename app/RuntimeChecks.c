#include "RuntimeChecks.h"
#include "cmsis_gcc.h"

/** Number of received CAN frames */
volatile uint64_t CanAbs_FrameCount = 0;

/** Number of dropped frames read from harware buffer */
volatile uint64_t CanAbs_FrameDropCount = 0;

/** Number of frames written to FDCAN msg port */
volatile uint64_t CanBridgeTask_FrameCount = 0;

/** Number of frames read from FDCAN msg port */
volatile uint64_t CanLogManager_FrameCount = 0;

/** Number of frames written to SD card staging buffer */
volatile uint64_t CanLogBuffer_FrameCount1 = 0;

/** Number of frames read from SD card staging buffer */
volatile uint64_t CanLogBuffer_FrameCount2 = 0;

/** Number of blocks sucessfully written to SD card (no SPI op error) */
volatile uint64_t CanLogBuffer_BlockCount = 0;

volatile uint64_t FcdanMsgPort_FrameDropCount = 0;

void RuntimeChecks_Init(void)
{
    CanAbs_FrameCount = 0;
    CanAbs_FrameDropCount = 0;
    CanBridgeTask_FrameCount = 0;
    CanLogManager_FrameCount = 0;
    CanLogBuffer_FrameCount1 = 0;
    CanLogBuffer_FrameCount2 = 0;
    CanLogBuffer_BlockCount = 0;
    FcdanMsgPort_FrameDropCount = 0;   
}

static RuntimeChecksErrorType CheckDropCounts(void)
{
    RuntimeChecksErrorType err = RUNTIMECHECKS_E_OK;

    if (0 != FcdanMsgPort_FrameDropCount)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanAbs_FrameDropCount) 
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
        __BKPT();
    }

    context->err = err;
}
