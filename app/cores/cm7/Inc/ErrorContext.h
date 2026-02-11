#pragma once

#include <stdint.h>
#include <stdio.h>

#define ERRORCONTEXT_FILE_NAME_LENGTH     (20U)
#define ERRORCONTEXT_FUNCTION_NAME_LENGTH (64U)

typedef uint32_t ErrorCodeType;

typedef struct
{
    ErrorCodeType code;
    uint16_t line;
    const char file[ERRORCONTEXT_FILE_NAME_LENGTH];
    char function[ERRORCONTEXT_FUNCTION_NAME_LENGTH];
} ErrorContextType;
