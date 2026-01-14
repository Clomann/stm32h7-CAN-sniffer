#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "CommTypes.h"

/* Stub control helpers */
void reset_can_stubs(void);
void set_can_start_result(comm_status_t result);
void set_can_stop_result(comm_status_t result);
void set_can_state_off(bool isOff);
bool get_can_started(void);
bool get_can_stopped(void);

/* CAN abstraction API used by CanLogManager */
comm_status_t CanAbs_Start_Can1(void);
comm_status_t CanAbs_Start_Can2(void);
comm_status_t CanAbs_Stop_Can1(void);
comm_status_t CanAbs_Stop_Can2(void);
comm_status_t CanAbs_IsStateOff_Can1(bool *isOff);
comm_status_t CanAbs_IsStateOff_Can2(bool *isOff);
void CanAbs_Drain(void);
