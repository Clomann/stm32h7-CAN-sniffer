#pragma once

#include <stdint.h>

typedef struct
{
    uint32_t baudrate;
    uint8_t mode;
} CanCtrlCanType;

typedef struct
{
    CanCtrlCanType can1;
    CanCtrlCanType can2;
    _Bool sendingActive;
} CanCtrlDataType;

void appFdcanInit(CanCtrlDataType *data);
void appFdcanPoll(_Bool sendingActive);

void appCanCtrlSetBaudrate(CanCtrlDataType *data);
void appCanCtrlSetMode(CanCtrlDataType *data);

/* Hooks */
void CANCONTROL_ErrorHandlerHook(void) __attribute__((weak));
