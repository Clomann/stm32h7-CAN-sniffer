#pragma once

#include <stdint.h>
#include <stdio.h>

#define ERRORCONTEXT_FILE_NAME_LENGTH     (20U)
#define ERRORCONTEXT_FUNCTION_NAME_LENGTH (64U)

typedef uint32_t ErrorCodeType;
typedef uint32_t ErrorIdType;

typedef struct
{
    ErrorIdType id;
    ErrorCodeType code;
    uint32_t arg0;
    uint32_t arg1;
    uint16_t line;
    char file[ERRORCONTEXT_FILE_NAME_LENGTH];
    char function[ERRORCONTEXT_FUNCTION_NAME_LENGTH];
} ErrorContextType;
