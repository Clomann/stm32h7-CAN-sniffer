
/**
 * @file CanLogManager.h
 * @brief CAN Log Manager - Handles CAN message logging with file rotation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "FileHandler.h"
#include "CanLogManagerTypes.h"

#define PERSIST_CAN_LOG_FILE_HEAD_TAIL 0U
#define MAX_LOG_FILE_SIZE              (8U * 1024U)
#define MAX_LOG_INDEX                  (16U)

#define CAN_LOG_MAX_FILE_SIZE_KB 8U
#define CAN_LOG_MAX_FILE_SIZE    (CAN_LOG_MAX_FILE_SIZE_KB * 1024U)
#define CAN_LOG_MAX_INDEX        16U
#define CAN_LOG_FILENAME_MAX_LEN 255U
#define CAN_LOG_BASE_DIR         "/logs/"
#define CAN_LOG_FILE_PREFIX      "CAN.LOG"

FRESULT appCanLogHandlerInit(CanLogControlDataType *data);
void appCanLogHandlerPoll(CanLogControlDataType *data);
void appCanLogHandlerDeInit(CanLogControlDataType *data);

/* Initialize the global control data */
CanLogControlDataType *CanLogHandler_Init(uint8_t *mount_res, bool *run);

/* Hooks */
void CanLogFileManager_ErrorHandler(void);
