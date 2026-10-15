#pragma once

#include <stdint.h>

#define RUNTIMECHECKS_E_OK                       0
#define RUNTIMECHECKS_E_NOT_OK                   1
#define RUNTIMECHECKS_E_PLAUSIBILITY_FRAME_COUNT 2
#define RUNTIMECHECKS_E_FRAMES_DROPPED           3

typedef uint8_t RuntimeChecksErrorType;

typedef struct
{
    RuntimeChecksErrorType err;
} RuntimeChecksContextType;

typedef struct
{
    uint64_t can_abs_frame_count;
    uint64_t can_abs_frame_drop_count;
    uint64_t can_abs_can1_rx_frame_drop_count;
    uint64_t can_abs_can2_rx_frame_drop_count;
    uint64_t can_bridge_task_frame_count;
    uint64_t can_log_manager_frame_count;
    uint64_t frame_delta1;
    uint64_t can_log_manager_frame_drop_count1;
    uint64_t can_log_manager_frame_drop_count2;
    uint64_t can_log_manager_can1_missing_count;
    uint64_t can_log_manager_can2_missing_count;
    uint64_t can_log_manager_can1_missing_ids_count;
    uint64_t can_log_manager_can2_missing_ids_count;
    uint64_t can_log_buffer_frame_count1;
    uint64_t can_log_buffer_frame_count2;
    uint64_t can_log_buffer_frame_drop_count;
    uint64_t can_log_buffer_block_count;
    uint64_t fdcan_msg_port_frame_drop_count;
    uint64_t error_handler_calls;
} RuntimeChecksDiagnosticsType;

/** Number of received CAN frames */
extern volatile uint64_t CanAbs_FrameCount;

extern volatile uint64_t CanAbs_FrameDropCount;

extern volatile uint64_t CanAbs_CAN1_Rx_FrameDropCount;

extern volatile uint64_t CanAbs_CAN2_Rx_FrameDropCount;

/** Number of frames written to FDCAN msg port */
extern volatile uint64_t CanBridgeTask_FrameCount;

/** Number of frames read from FDCAN msg port */
extern volatile uint64_t CanLogManager_FrameCount;

/** Number of frames dropped trying to add to the buffer in CanLogManager */
extern volatile uint64_t CanLogManager_FrameDropCount1;

/** Number of frames dropped because downstream log storage did not drain the staging buffer in time */
extern volatile uint64_t CanLogManager_FrameDropCount2;

/** Number of frames dropped based on internal E2E sequenceing for CAN 1 */
extern volatile uint64_t CanLogManager_CAN1_MissingCount;

/** Number of frames dropped based on internal E2E sequenceing for CAN 2 */
extern volatile uint64_t CanLogManager_CAN2_MissingCount;

/** Number of CAN ID test-sequence errors on CAN 1.
 *  Repeated IDs are treated as sequence errors.
 */
extern volatile uint64_t CanLogManager_CAN1_MissingIdsCount;

/** Number of CAN ID test-sequence errors on CAN 2.
 *  Repeated IDs are treated as sequence errors.
 */
extern volatile uint64_t CanLogManager_CAN2_MissingIdsCount;

/** Number of frames written to SD card staging buffer */
extern volatile uint64_t CanLogBuffer_FrameCount1;

/** Number of frames read from SD card staging buffer */
extern volatile uint64_t CanLogBuffer_FrameCount2;

/** Number of frames dropped between writing to and reading from buffer of CanLogBuffer */
extern volatile uint64_t CanLogBuffer_FrameDropCount;

/** Number of blocks sucessfully written to SD card (no SPI op error) */
extern volatile uint64_t CanLogBuffer_BlockCount;

/** Number of frames that were drop on full can frame buffer in message port */
extern volatile uint64_t FcdanMsgPort_FrameDropCount;

void RuntimeChecks_Init(void);

void RuntimeChecks_CheckFrameCounts(RuntimeChecksContextType *context);

uint8_t RuntimeChecks_GetDiagnostics(RuntimeChecksDiagnosticsType *diagnostics);
