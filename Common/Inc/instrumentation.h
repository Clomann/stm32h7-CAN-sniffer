#pragma once

#include <stdint.h>
#include "ErrorContext.h"

#define INSTR_ENABLED        1
#define INSTR_PERSIST_ACTIVE 1

#define INSTR_E_OK     0U
#define INSTR_E_NOT_OK 1U

typedef uint8_t InstrErrorType;

void Instrumentation_Init(void);

InstrErrorType
Instrumentation_SerializeHook(uint8_t *buf, uint32_t *size);

void Instrumentation_ErrorHandlerHook(ErrorContextType *context);
