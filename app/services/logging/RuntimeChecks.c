#include "RuntimeChecks.h"
#include "cmsis_gcc.h"

#include <stddef.h>

volatile uint64_t CanAbs_FrameCount                  = 0;
volatile uint64_t CanAbs_FrameDropCount              = 0;
volatile uint64_t CanAbs_CAN1_Rx_FrameDropCount      = 0;
volatile uint64_t CanAbs_CAN2_Rx_FrameDropCount      = 0;
volatile uint64_t CanBridgeTask_FrameCount           = 0;
volatile uint64_t CanLogManager_FrameCount           = 0;
volatile uint64_t CanLogManager_FrameDropCount1      = 0;
volatile uint64_t CanLogManager_FrameDropCount2      = 0;
volatile uint64_t CanLogManager_CAN1_MissingCount    = 0;
volatile uint64_t CanLogManager_CAN2_MissingCount    = 0;
volatile uint64_t CanLogManager_CAN1_MissingIdsCount = 0;
volatile uint64_t CanLogManager_CAN2_MissingIdsCount = 0;
volatile uint64_t CanLogBuffer_FrameCount1           = 0;
volatile uint64_t CanLogBuffer_FrameCount2           = 0;
volatile uint64_t CanLogBuffer_FrameDropCount        = 0;
volatile uint64_t CanLogBuffer_BlockCount            = 0;
volatile uint64_t FcdanMsgPort_FrameDropCount        = 0;
volatile uint64_t FrameDelta1                        = 0;
volatile uint64_t ErrorHandlerCalls                  = 0;

static void RuntimeChecks_ErrorHandler(void)
{
    ErrorHandlerCalls++;
}

void RuntimeChecks_Init(void)
{
    CanAbs_FrameCount                  = 0;
    CanAbs_FrameDropCount              = 0;
    CanAbs_CAN1_Rx_FrameDropCount      = 0;
    CanAbs_CAN2_Rx_FrameDropCount      = 0;
    CanBridgeTask_FrameCount           = 0;
    CanLogManager_FrameCount           = 0;
    CanLogManager_FrameDropCount1      = 0;
    CanLogManager_FrameDropCount2      = 0;
    CanLogManager_CAN1_MissingCount    = 0;
    CanLogManager_CAN2_MissingCount    = 0;
    CanLogManager_CAN1_MissingIdsCount = 0;
    CanLogManager_CAN2_MissingIdsCount = 0;
    CanLogBuffer_FrameCount1           = 0;
    CanLogBuffer_FrameCount2           = 0;
    CanLogBuffer_FrameDropCount        = 0;
    CanLogBuffer_BlockCount            = 0;
    FcdanMsgPort_FrameDropCount        = 0;
    ErrorHandlerCalls                  = 0;
}

static RuntimeChecksErrorType CheckDropCounts(void)
{
    RuntimeChecksErrorType err = RUNTIMECHECKS_E_OK;

    FrameDelta1 = CanBridgeTask_FrameCount - CanLogManager_FrameCount;

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
    else if (FrameDelta1 > 1024U)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (CanLogBuffer_FrameDropCount > 1024U)
    {
        // only for monitoring
    }
    else if (0 != CanLogManager_FrameDropCount1)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanLogManager_FrameDropCount2)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanLogManager_CAN1_MissingCount
             || 0 != CanLogManager_CAN2_MissingCount)
    {
        err = RUNTIMECHECKS_E_FRAMES_DROPPED;
    }
    else if (0 != CanLogManager_CAN1_MissingIdsCount
             || 0 != CanLogManager_CAN2_MissingIdsCount)
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
        (void)context;
        RuntimeChecks_ErrorHandler();
    }

    context->err = err;
}

uint8_t RuntimeChecks_GetDiagnostics(RuntimeChecksDiagnosticsType *diagnostics)
{
    if (NULL == diagnostics)
    {
        return 1U;
    }

    diagnostics->can_abs_frame_count      = CanAbs_FrameCount;
    diagnostics->can_abs_frame_drop_count = CanAbs_FrameDropCount;
    diagnostics->can_abs_can1_rx_frame_drop_count =
        CanAbs_CAN1_Rx_FrameDropCount;
    diagnostics->can_abs_can2_rx_frame_drop_count =
        CanAbs_CAN2_Rx_FrameDropCount;
    diagnostics->can_bridge_task_frame_count = CanBridgeTask_FrameCount;
    diagnostics->can_log_manager_frame_count = CanLogManager_FrameCount;
    diagnostics->frame_delta1                = FrameDelta1;
    diagnostics->can_log_manager_frame_drop_count1 =
        CanLogManager_FrameDropCount1;
    diagnostics->can_log_manager_frame_drop_count2 =
        CanLogManager_FrameDropCount2;
    diagnostics->can_log_manager_can1_missing_count =
        CanLogManager_CAN1_MissingCount;
    diagnostics->can_log_manager_can2_missing_count =
        CanLogManager_CAN2_MissingCount;
    diagnostics->can_log_manager_can1_missing_ids_count =
        CanLogManager_CAN1_MissingIdsCount;
    diagnostics->can_log_manager_can2_missing_ids_count =
        CanLogManager_CAN2_MissingIdsCount;
    diagnostics->can_log_buffer_frame_count1     = CanLogBuffer_FrameCount1;
    diagnostics->can_log_buffer_frame_count2     = CanLogBuffer_FrameCount2;
    diagnostics->can_log_buffer_frame_drop_count = CanLogBuffer_FrameDropCount;
    diagnostics->can_log_buffer_block_count      = CanLogBuffer_BlockCount;
    diagnostics->fdcan_msg_port_frame_drop_count = FcdanMsgPort_FrameDropCount;
    diagnostics->error_handler_calls             = ErrorHandlerCalls;

    return 0U;
}
