#pragma once

#include <stdint.h>

#define CLM_PARAMETER_ID_CAN1_BAUDRATE 1
#define CLM_PARAMETER_ID_CAN2_BAUDRATE 2
#define CLM_PARAMETER_ID_MAX           3

typedef uint8_t ClmParameterIdType;

typedef enum
{
    CAN_LOG_OK = 0,
    CAN_LOG_NOT_OK,
    CAN_LOG_ERR_INIT_FAILED,
    CAN_LOG_ERR_FILE_OPEN,
    CAN_LOG_ERR_FILE_WRITE,
    CAN_LOG_ERR_FILE_ROTATE,
    CAN_LOG_ERR_INVALID_PARAM,
    CAN_LOG_ERR_INVALID_POINTER,
    CAN_LOG_ERR_NO_SPACE
} CanLogResult;

typedef struct CanLogControlDataType CanLogControlDataType;
