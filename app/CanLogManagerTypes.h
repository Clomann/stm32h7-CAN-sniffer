#pragma once

typedef enum
{
    CAN_LOG_OK = 0,
    CAN_LOG_ERR_INIT_FAILED,
    CAN_LOG_ERR_FILE_OPEN,
    CAN_LOG_ERR_FILE_WRITE,
    CAN_LOG_ERR_FILE_ROTATE,
    CAN_LOG_ERR_INVALID_PARAM,
    CAN_LOG_ERR_NO_SPACE
} CanLogResult;

typedef struct CanLogControlDataType CanLogControlDataType;
