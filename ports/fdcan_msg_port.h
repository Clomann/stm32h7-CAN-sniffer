#pragma once
#include "CanAbs.h"

void fdcan_msg_port_receive(FDCAN_ClassicFrame *frame);

void fdcan_msg_port_init(void);

size_t fdcan_msg_port_read(FDCAN_ClassicFrame **dst, uint32_t milliSeconds);