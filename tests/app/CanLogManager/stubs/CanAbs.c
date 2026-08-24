#include "CanAbs.h"

#include <string.h>

bool mock_can_started               = false;
bool mock_can_stopped               = false;
comm_status_t mock_can_start_result = COMM_SUCCESS;
comm_status_t mock_can_stop_result  = COMM_SUCCESS;
bool mock_can_state_off             = false;

void reset_can_stubs(void)
{
    mock_can_started      = false;
    mock_can_stopped      = false;
    mock_can_start_result = COMM_SUCCESS;
    mock_can_stop_result  = COMM_SUCCESS;
    mock_can_state_off    = false;
}

void set_can_start_result(comm_status_t result)
{
    mock_can_start_result = result;
}

void set_can_stop_result(comm_status_t result)
{
    mock_can_stop_result = result;
}

void set_can_state_off(bool isOff)
{
    mock_can_state_off = isOff;
}

bool get_can_started(void)
{
    return mock_can_started;
}

bool get_can_stopped(void)
{
    return mock_can_stopped;
}

comm_status_t CanAbs_Start_Can1(void)
{
    if (mock_can_start_result == COMM_SUCCESS)
    {
        mock_can_started = true;
    }
    return mock_can_start_result;
}

comm_status_t CanAbs_Start_Can2(void)
{
    if (mock_can_start_result == COMM_SUCCESS)
    {
        mock_can_started = true;
    }
    return mock_can_start_result;
}

comm_status_t CanAbs_Stop_Can1(void)
{
    if (mock_can_stop_result == COMM_SUCCESS)
    {
        mock_can_stopped = true;
    }
    return mock_can_stop_result;
}

comm_status_t CanAbs_Stop_Can2(void)
{
    if (mock_can_stop_result == COMM_SUCCESS)
    {
        mock_can_stopped = true;
    }
    return mock_can_stop_result;
}

comm_status_t CanAbs_IsStateOff_Can1(bool *isOff)
{
    if (isOff)
    {
        *isOff = mock_can_state_off;
    }
    return COMM_SUCCESS;
}

comm_status_t CanAbs_IsStateOff_Can2(bool *isOff)
{
    if (isOff)
    {
        *isOff = mock_can_state_off;
    }
    return COMM_SUCCESS;
}

uint8_t CanAbs_GetRxHighWater_Can1(uint32_t *frames)
{
    if (frames)
    {
        *frames = 0U;
    }
    return 0U;
}

uint8_t CanAbs_GetRxHighWater_Can2(uint32_t *frames)
{
    if (frames)
    {
        *frames = 0U;
    }
    return 0U;
}

uint32_t CanAbs_GetRxBufferCapacity(void)
{
    return 0U;
}
uint8_t CanAbs_GetStaticTxReplayStats_Can1(FdcanStaticTxReplayStatsType *stats)
{
    if (stats)
    {
        memset(stats, 0, sizeof(*stats));
    }
    return 0U;
}

uint8_t CanAbs_GetStaticTxReplayStats_Can2(FdcanStaticTxReplayStatsType *stats)
{
    if (stats)
    {
        memset(stats, 0, sizeof(*stats));
    }
    return 0U;
}

uint8_t CanAbs_GetFdcanHealth_Can1(CanAbsFdcanHealthStatsType *stats)
{
    if (stats)
    {
        memset(stats, 0, sizeof(*stats));
    }
    return 0U;
}

uint8_t CanAbs_GetFdcanHealth_Can2(CanAbsFdcanHealthStatsType *stats)
{
    if (stats)
    {
        memset(stats, 0, sizeof(*stats));
    }
    return 0U;
}

void CanAbs_ResetFdcanHealth(void)
{
    /* no-op in stub */
}

uint8_t CanAbs_GetHwRxFifoHighWater_Can1(uint32_t *frames)
{
    if (frames)
    {
        *frames = 0U;
    }
    return 0U;
}

uint8_t CanAbs_GetHwRxFifoHighWater_Can2(uint32_t *frames)
{
    if (frames)
    {
        *frames = 0U;
    }
    return 0U;
}

uint32_t CanAbs_GetHwRxFifoCapacity(void)
{
    return 0U;
}

void CanAbs_Drain(void)
{
    /* no-op in stub */
}
