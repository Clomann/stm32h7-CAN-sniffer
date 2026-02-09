#pragma once

void SdBridgeTask_ActionHook(void);

static inline void SdBridgeTask_Init(void)
{
}

static inline void SdBridgeTask(void *arg)
{
    (void)arg;
}

static inline void SdBridgeTask_Notify(void)
{
    /* In unit tests, execute the flush immediately. */
    SdBridgeTask_ActionHook();
}
