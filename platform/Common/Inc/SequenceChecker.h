#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ScSequenceType_s
{
    bool initialized;
    uint32_t previous_id;
    uint32_t gap_count;
    uint32_t missing_count;
    uint32_t wrap_index;
} ScSequenceType;

#define SC_E_OK          ((ScStatusType)0U)
#define SC_E_NOT_OK      ((ScStatusType) - 1U)
#define SC_E_INIT        ((ScStatusType) - 2U)
#define SC_E_INV_PARAM   ((ScStatusType) - 3U)
#define SC_E_INV_POINTER ((ScStatusType) - 4U)
/** @brief invalid call sequence of modules functions */
#define SC_E_SEQUENCE ((ScStatusType) - 5U)

typedef int32_t ScStatusType;

ScStatusType ScInit(ScSequenceType *seq, uint32_t index, uint32_t wrap_index);

ScStatusType ScDeInit(ScSequenceType *seq);

ScStatusType ScCheckSequence(ScSequenceType *seq, uint32_t new_index);

ScStatusType ScGetMissingCount(const ScSequenceType *seq, uint32_t *const cnt);

ScStatusType ScReset(ScSequenceType *seq, uint32_t index);
