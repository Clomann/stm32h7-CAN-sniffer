#pragma once

#include "nvic_irg_config.h"
#include "Core0TasksCfg.h"

extern volatile uint32_t CanBridgeTask_FrameCount;

void CanBridgeTaskInit(void);

void CanBridgeTask(void *arg);
