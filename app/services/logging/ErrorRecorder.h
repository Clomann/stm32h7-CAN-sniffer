#pragma once

#include <stdint.h>

#include "ErrorContext.h"

#define ERROR_RECORDER_RECENT_COUNT 10U

typedef struct
{
    uint32_t sequence;
    ErrorContextType context;
} ErrorRecorderEntryType;

typedef struct
{
    uint32_t total_count;
    uint32_t overwritten_count;
    uint8_t first_valid;
    uint8_t last_valid;
    uint8_t recent_count;
    ErrorRecorderEntryType first;
    ErrorRecorderEntryType last;
    ErrorRecorderEntryType recent[ERROR_RECORDER_RECENT_COUNT];
} ErrorRecorderSnapshotType;

void ErrorRecorder_Clear(void);
void ErrorRecorder_Record(const ErrorContextType *context);
uint8_t ErrorRecorder_GetSnapshot(ErrorRecorderSnapshotType *snapshot);
