#pragma once

static inline void SdBridgeTask_Init(void)
{
}

static inline void SdBridgeTask(void *arg)
{
    (void)arg;
}

static inline void SdBridgeTask_Notify(void)
{
}

void SdBridgeTask_ActionHook(void);
