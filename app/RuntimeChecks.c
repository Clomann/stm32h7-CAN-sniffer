#pragma once 

#include "RuntimeChecks.h"

/** Number of received CAN frames */
volatile uint64_t CanAbs_FrameCount = 0;

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

void RuntimeChecks_CheckFrameCounts(RuntimeChecksContextType *context)
{
    (void) context;
}
