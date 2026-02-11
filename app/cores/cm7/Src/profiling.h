#pragma once

extern uint64_t FDCAN_GetTimestampHook(void);

void force_profiling_link(void);

uint64_t Profiling_GetTimestamp(void);

void update_task_stats(void);
