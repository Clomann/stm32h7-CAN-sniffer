#pragma once 

#include <stdint.h>

#define RUNTIMECHECKS_E_OK                       0
#define RUNTIMECHECKS_E_NOT_OK                   1
#define RUNTIMECHECKS_E_PLAUSIBILITY_FRAME_COUNT 2
#define RUNTIMECHECKS_E_FRAMES_DROPPED           3

typedef uint8_t RuntimeChecksErrorType;

typedef struct {
    RuntimeChecksErrorType err;
} RuntimeChecksContextType;

/** Number of received CAN frames */
extern volatile uint64_t CanAbs_FrameCount;

extern volatile uint64_t CanAbs_FrameDropCount;

/** Number of frames written to FDCAN msg port */
extern volatile uint64_t CanBridgeTask_FrameCount;

/** Number of frames read from FDCAN msg port */
extern volatile uint64_t CanLogManager_FrameCount;

/** Number of frames written to SD card staging buffer */
extern volatile uint64_t CanLogBuffer_FrameCount1;

/** Number of frames read from SD card staging buffer */
extern volatile uint64_t CanLogBuffer_FrameCount2;

/** Number of blocks sucessfully written to SD card (no SPI op error) */
extern volatile uint64_t CanLogBuffer_BlockCount;

/** Number of frames that were drop on full can frame buffer in message port */
extern volatile uint64_t FcdanMsgPort_FrameDropCount;

void RuntimeChecks_Init(void);

void RuntimeChecks_CheckFrameCounts(RuntimeChecksContextType *context);
