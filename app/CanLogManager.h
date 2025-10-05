
/**
 * @file CanLogManager.h
 * @brief CAN Log Manager - Handles CAN message logging with file rotation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "FileHandler.h"
#include "CanLogManagerTypes.h"

#define CLUSTER_SIZE                   (32768U) 
#define PERSIST_CAN_LOG_FILE_HEAD_TAIL 0U
#define MAX_LOG_FILE_SIZE              (1U * 1024U * 1024U)
#define MAX_LOG_INDEX                  (32U)

#define CAN_LOG_MAX_FILE_SIZE_KB 16U
#define CAN_LOG_MAX_FILE_SIZE    (CAN_LOG_MAX_FILE_SIZE_KB * 1024U)
#define CAN_LOG_MAX_INDEX        16U
#define CAN_LOG_FILENAME_MAX_LEN 255U
#define CAN_LOG_BASE_DIR         "/logs/"
#define CAN_LOG_FILE_PREFIX      "CAN.LOG"
#define CANLOGMANAGER_CLEAR_ALL_LOGS 0U
#define PREALLOCATE_LOG_FILES         1U

FRESULT appCanLogHandlerInit(CanLogControlDataType *data);
void appCanLogHandlerPoll(CanLogControlDataType *data);
void appCanLogHandlerDeInit(CanLogControlDataType *data);

/* Initialize the global control data */
CanLogControlDataType *CanLogHandler_Init(uint8_t *mount_res, bool *run, bool *commit);

/* Hooks */
void CanLogFileManager_ErrorHandler(void);
