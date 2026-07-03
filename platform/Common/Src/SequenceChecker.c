#include "SequenceChecker.h"

ScStatusType ScInit(ScSequenceType *seq, uint32_t index, uint32_t wrap_index)
{
    if (NULL == seq)
    {
        return SC_E_INV_POINTER;
    }

    if ((0U == wrap_index) || (index > wrap_index))
    {
        return SC_E_INV_PARAM;
    }

    seq->previous_id   = index;
    seq->gap_count     = 0U;
    seq->missing_count = 0U;
    seq->wrap_index    = wrap_index;
    seq->initialized   = true;

    return SC_E_OK;
}

ScStatusType ScDeInit(ScSequenceType *seq)
{
    if (seq == NULL)
    {
        return SC_E_INV_POINTER;
    }

    if (false == seq->initialized)
    {
        return SC_E_SEQUENCE;
    }

    seq->previous_id   = 0U;
    seq->gap_count     = 0U;
    seq->missing_count = 0U;
    seq->wrap_index    = 0U;
    seq->initialized   = false;

    return SC_E_OK;
}

ScStatusType ScCheckSequence(ScSequenceType *seq, uint32_t new_index)
{
    ScStatusType res = SC_E_OK;
    uint32_t delta   = 0U;

    if (NULL == seq)
    {
        return SC_E_INV_POINTER;
    }

    if (false == seq->initialized)
    {
        return SC_E_INIT;
    }

    if (new_index > seq->wrap_index)
    {
        return SC_E_INV_PARAM;
    }

    if (new_index == seq->previous_id)
    {
        seq->gap_count = 0U;
        return SC_E_SEQUENCE;
    }

    if (new_index > seq->previous_id)
    {
        delta = new_index - seq->previous_id;
    }
    else
    {
        delta = (seq->wrap_index - seq->previous_id) + new_index + 1U;
    }

    if (1U == delta)
    {
        // contiguous frame
        seq->gap_count = 0U;
    }
    else if (1U < delta)
    {
        // missing = delta - 1
        seq->gap_count = delta - 1U;
    }

    if (UINT32_MAX - seq->missing_count >= seq->gap_count)
    {
        seq->missing_count += seq->gap_count;
    }
    else
    {
        seq->missing_count = UINT32_MAX;
    }

    seq->previous_id = new_index;

    return res;
}

ScStatusType ScGetMissingCount(const ScSequenceType *seq, uint32_t *const cnt)
{
    if (NULL == seq || NULL == cnt)
    {
        return SC_E_INV_POINTER;
    }

    if (false == seq->initialized)
    {
        return SC_E_INIT;
    }

    *cnt = seq->missing_count;

    return SC_E_OK;
}

ScStatusType ScReset(ScSequenceType *seq, uint32_t index)
{
    if (NULL == seq)
    {
        return SC_E_INV_POINTER;
    }

    if (false == seq->initialized)
    {
        return SC_E_SEQUENCE;
    }
    
    if ((0U == seq->wrap_index) || (index > seq->wrap_index))
    {
        return SC_E_INV_PARAM;
    }

    seq->previous_id   = index;
    seq->gap_count     = 0U;
    seq->missing_count = 0U;

    return SC_E_OK;
}
