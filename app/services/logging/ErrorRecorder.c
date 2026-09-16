#include "ErrorRecorder.h"

#include <stddef.h>
#include <string.h>

static ErrorRecorderEntryType FirstError;
static ErrorRecorderEntryType LastError;
static ErrorRecorderEntryType RecentErrors[ERROR_RECORDER_RECENT_COUNT];
static uint32_t TotalCount;
static uint8_t RecentCount;
static uint8_t RecentNext;

void ErrorRecorder_Clear(void)
{
    memset(&FirstError, 0, sizeof(FirstError));
    memset(&LastError, 0, sizeof(LastError));
    memset(RecentErrors, 0, sizeof(RecentErrors));
    TotalCount  = 0U;
    RecentCount = 0U;
    RecentNext  = 0U;
}

void ErrorRecorder_Record(const ErrorContextType *context)
{
    ErrorRecorderEntryType entry;

    if (NULL == context)
    {
        return;
    }

    memset(&entry, 0, sizeof(entry));
    entry.sequence = TotalCount + 1U;
    entry.context  = *context;

    if (0U == TotalCount)
    {
        FirstError = entry;
    }

    LastError                = entry;
    RecentErrors[RecentNext] = entry;
    RecentNext++;
    if (RecentNext >= ERROR_RECORDER_RECENT_COUNT)
    {
        RecentNext = 0U;
    }

    if (RecentCount < ERROR_RECORDER_RECENT_COUNT)
    {
        RecentCount++;
    }

    TotalCount++;
}

uint8_t ErrorRecorder_GetSnapshot(ErrorRecorderSnapshotType *snapshot)
{
    uint8_t start;

    if (NULL == snapshot)
    {
        return 1U;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    snapshot->total_count  = TotalCount;
    snapshot->recent_count = RecentCount;
    snapshot->overwritten_count =
        (TotalCount > (uint32_t)RecentCount) ? (TotalCount - RecentCount) : 0U;

    if (TotalCount > 0U)
    {
        snapshot->first_valid = 1U;
        snapshot->last_valid  = 1U;
        snapshot->first       = FirstError;
        snapshot->last        = LastError;
    }

    start = (RecentCount == ERROR_RECORDER_RECENT_COUNT) ? RecentNext : 0U;
    for (uint8_t i = 0U; i < RecentCount; i++)
    {
        uint8_t index =
            (uint8_t)((uint32_t)(start + i) % ERROR_RECORDER_RECENT_COUNT);
        snapshot->recent[i] = RecentErrors[index];
    }

    return 0U;
}
