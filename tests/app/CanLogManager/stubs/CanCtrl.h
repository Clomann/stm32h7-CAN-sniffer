#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t baudrate;
    uint8_t mode;
} CanCtrlCanType;

typedef struct
{
    CanCtrlCanType can1;
    CanCtrlCanType can2;
    bool sendingActive;
} CanCtrlDataType;

static inline void appFdcanInit(CanCtrlDataType *data)
{
    (void)data;
}

static inline void appFdcanPoll(bool sendingActive)
{
    (void)sendingActive;
}

static inline void appCanCtrlSetBaudrate(CanCtrlDataType *data)
{
    (void)data;
}

static inline void appCanCtrlSetMode(CanCtrlDataType *data)
{
    (void)data;
}

static inline void CANCONTROL_ErrorHandlerHook(void)
{
}
