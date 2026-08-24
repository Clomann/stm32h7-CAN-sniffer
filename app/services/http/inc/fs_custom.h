#pragma once

#include <stdint.h>

typedef struct
{
    uint64_t can_rx_frames;
    uint64_t can_rx_dropped_frames;
    uint64_t can1_rx_dropped_frames;
    uint64_t can2_rx_dropped_frames;
    uint64_t bridge_ingress_frames;
    uint64_t logger_ingress_frames;
    uint64_t bridge_logger_backlog_frames;
    uint64_t logger_buffer_rejected_frames;
    uint64_t logger_buffer_full_frames;
    uint64_t can1_sequence_gap_frames;
    uint64_t can2_sequence_gap_frames;
    uint64_t can1_id_sequence_errors;
    uint64_t can2_id_sequence_errors;
    uint64_t sd_buffer_ingress_frames;
    uint64_t sd_buffer_egress_frames;
    uint64_t sd_buffer_dropped_frames;
    uint64_t sd_blocks_written;
    uint64_t message_port_dropped_frames;
    uint64_t runtime_check_error_calls;
    uint64_t can1_rx_fifo0_lost_events;
    uint64_t can2_rx_fifo0_lost_events;
    uint64_t can1_protocol_error_events;
    uint64_t can2_protocol_error_events;
    uint64_t can1_error_warning_events;
    uint64_t can2_error_warning_events;
    uint64_t can1_error_passive_events;
    uint64_t can2_error_passive_events;
    uint64_t can1_bus_off_events;
    uint64_t can2_bus_off_events;
} FsCustomDebugSnapshotType;

typedef uint8_t (*FsCustomDebugProviderReadFn)(
    void *context,
    FsCustomDebugSnapshotType *snapshot
);

typedef struct
{
    void *context;
    FsCustomDebugProviderReadFn read;
} FsCustomDebugProviderType;

typedef struct
{
    uint32_t protocol_status;
    uint32_t error_counter;
    uint32_t rx_fifo0_status;
    uint32_t interrupt_flags;
    uint32_t protocol_status_latched;
    uint32_t error_counter_latched;
    uint32_t rx_fifo0_status_latched;
    uint32_t interrupt_flags_latched;
    uint64_t rx_fifo0_lost_events;
    uint64_t protocol_error_events;
    uint64_t error_warning_events;
    uint64_t error_passive_events;
    uint64_t bus_off_events;
} FsCustomFdcanHealthStatsType;

typedef struct
{
    uint32_t rb1_bytes_capacity;
    uint32_t rb1_bytes_now;
    uint32_t rb1_bytes_highwater;
    uint32_t rb1_bytes_min_since_status;
    uint32_t rb1_bytes_max_since_status;
    uint32_t rb1_bytes_at_last_sd_block_start;
    uint32_t rb1_bytes_at_last_sd_block_end_before_consume;
    uint32_t rb1_bytes_at_last_sd_block_end_after_consume;
    uint32_t sd_block_attempts_since_status;
    uint32_t sd_blocks_written_since_status;
    uint32_t sd_block_errors_since_status;
    uint32_t sd_last_block_frames;
    uint32_t sd_write_last_us;
    uint32_t sd_sync_last_us;
    uint32_t sd_store_block_last_us;
    uint32_t sd_write_max_since_status_us;
    uint32_t sd_sync_max_since_status_us;
    uint32_t sd_store_block_max_since_status_us;
    uint32_t sd_store_block_avg_since_status_us;
} FsCustomCanLogBufferTelemetryType;

/* Shim functions that need to be implemented by the caller */
uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index);
uint8_t FsCustom_GetCanLogCapacity(uint32_t *capacity);
uint8_t FsCustom_GetBusloadCan1(float *busload);
uint8_t FsCustom_GetBusloadCan2(float *busload);
uint8_t FsCustom_GetRb1BytesHighWater(uint32_t *bytes);
uint8_t FsCustom_GetCanAbsRxHighWaterCan1(uint32_t *frames);
uint8_t FsCustom_GetCanAbsRxHighWaterCan2(uint32_t *frames);
uint8_t FsCustom_GetCanAbsRxCapacity(uint32_t *frames);
uint8_t FsCustom_GetStaticTxReplayStatsCan1(
    uint32_t *requests,
    uint32_t *completed,
    uint32_t *irqs
);
uint8_t FsCustom_GetStaticTxReplayStatsCan2(
    uint32_t *requests,
    uint32_t *completed,
    uint32_t *irqs
);
uint8_t FsCustom_GetCanAbsFdcanHealthCan1(FsCustomFdcanHealthStatsType *stats);
uint8_t FsCustom_GetCanAbsFdcanHealthCan2(FsCustomFdcanHealthStatsType *stats);
uint8_t FsCustom_GetFdcanMsgPortHighWater(uint32_t *bytes);
uint8_t FsCustom_GetFdcanMsgPortCapacity(uint32_t *bytes);
uint8_t FsCustom_GetCanLogFrameCount(uint64_t *count);
uint8_t FsCustom_GetPreallocErrorFlag(uint8_t *flag);
uint8_t FsCustom_GetCanLogSdTimingMaxUs(
    uint32_t *write_us,
    uint32_t *sync_us,
    uint32_t *store_block_us
);
uint8_t
FsCustom_GetCanLogBufferTelemetry(FsCustomCanLogBufferTelemetryType *telemetry);
uint8_t FsCustom_SetDebugProvider(const FsCustomDebugProviderType *provider);

uint8_t FsCustom_IsTracerRunning(uint8_t *running);
_Bool FsCustom_IsAnyFrameLostFlag(void);
