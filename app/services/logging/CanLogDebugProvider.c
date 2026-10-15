#include "CanLogDebugProvider.h"

#include "RuntimeChecks.h"
#include "fs_custom.h"

#include <stddef.h>

static uint8_t
CanLogDebugProvider_Read(void *context, FsCustomDebugSnapshotType *snapshot)
{
    RuntimeChecksDiagnosticsType diagnostics;
    FsCustomFdcanHealthStatsType can1_health = {0U};
    FsCustomFdcanHealthStatsType can2_health = {0U};

    (void)context;

    if (NULL == snapshot)
    {
        return 1U;
    }

    if (0U != RuntimeChecks_GetDiagnostics(&diagnostics))
    {
        return 1U;
    }

    (void)FsCustom_GetCanAbsFdcanHealthCan1(&can1_health);
    (void)FsCustom_GetCanAbsFdcanHealthCan2(&can2_health);

    snapshot->can_rx_frames         = diagnostics.can_abs_frame_count;
    snapshot->can_rx_dropped_frames = diagnostics.can_abs_frame_drop_count;
    snapshot->can1_rx_dropped_frames =
        diagnostics.can_abs_can1_rx_frame_drop_count;
    snapshot->can2_rx_dropped_frames =
        diagnostics.can_abs_can2_rx_frame_drop_count;
    snapshot->bridge_ingress_frames = diagnostics.can_bridge_task_frame_count;
    snapshot->logger_ingress_frames = diagnostics.can_log_manager_frame_count;
    snapshot->bridge_logger_backlog_frames = diagnostics.frame_delta1;
    snapshot->logger_buffer_rejected_frames =
        diagnostics.can_log_manager_frame_drop_count1;
    snapshot->logger_buffer_full_frames =
        diagnostics.can_log_manager_frame_drop_count2;
    snapshot->can1_sequence_gap_frames =
        diagnostics.can_log_manager_can1_missing_count;
    snapshot->can2_sequence_gap_frames =
        diagnostics.can_log_manager_can2_missing_count;
    snapshot->can1_id_sequence_errors =
        diagnostics.can_log_manager_can1_missing_ids_count;
    snapshot->can2_id_sequence_errors =
        diagnostics.can_log_manager_can2_missing_ids_count;
    snapshot->sd_buffer_ingress_frames =
        diagnostics.can_log_buffer_frame_count1;
    snapshot->sd_buffer_egress_frames = diagnostics.can_log_buffer_frame_count2;
    snapshot->sd_buffer_dropped_frames =
        diagnostics.can_log_buffer_frame_drop_count;
    snapshot->sd_blocks_written = diagnostics.can_log_buffer_block_count;
    snapshot->message_port_dropped_frames =
        diagnostics.fdcan_msg_port_frame_drop_count;
    snapshot->runtime_check_error_calls  = diagnostics.error_handler_calls;
    snapshot->can1_rx_fifo0_lost_events  = can1_health.rx_fifo0_lost_events;
    snapshot->can2_rx_fifo0_lost_events  = can2_health.rx_fifo0_lost_events;
    snapshot->can1_protocol_error_events = can1_health.protocol_error_events;
    snapshot->can2_protocol_error_events = can2_health.protocol_error_events;
    snapshot->can1_error_warning_events  = can1_health.error_warning_events;
    snapshot->can2_error_warning_events  = can2_health.error_warning_events;
    snapshot->can1_error_passive_events  = can1_health.error_passive_events;
    snapshot->can2_error_passive_events  = can2_health.error_passive_events;
    snapshot->can1_bus_off_events        = can1_health.bus_off_events;
    snapshot->can2_bus_off_events        = can2_health.bus_off_events;

    return 0U;
}

void CanLogDebugProvider_Register(void)
{
    FsCustomDebugProviderType provider = {
        .context = NULL,
        .read    = CanLogDebugProvider_Read,
    };

    (void)FsCustom_SetDebugProvider(&provider);
}
