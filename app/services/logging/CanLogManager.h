
/**
 * @file CanLogManager.h
 * @brief CAN Log Manager - Handles CAN message logging with file rotation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ErrorContext.h"

#include "FileHandler.h"
#include "CanLogManagerTypes.h"

#define CONCAT(a, b) a #b

#define CANLOGAMANGER_PERSIST_METADATA 1
#define CANLOGMANAGER_REOPEN_LOG_FILE  0

#define CLUSTER_SIZE                   (32768U)
#define PERSIST_CAN_LOG_FILE_HEAD_TAIL 0U
#ifdef MAX_LOG_FILE_COUNT
#define MAX_LOG_FILE_COUNT_FROM_COMPILER 1U
#else
#define MAX_LOG_FILE_COUNT_FROM_COMPILER 0U
#define MAX_LOG_FILE_COUNT               640U
#endif
#define MAX_LOG_FILE_SIZE (10U * 1024U * 1024U)
#define MAX_LOG_INDEX     (MAX_LOG_FILE_COUNT - 1U)

#define CAN_LOG_MAX_FILE_SIZE_KB     16U
#define CAN_LOG_MAX_FILE_SIZE        (CAN_LOG_MAX_FILE_SIZE_KB * 1024U)
#define CAN_LOG_MAX_INDEX            16U
#define CAN_LOG_FILENAME_MAX_LEN     255U
#define CAN_LOG_BASE_DIR             "/logs/"
#define CAN_LOG_FILE_PREFIX          "CAN.LOG"
#define CAN_LOG_META_FILENAME        "/logs_meta.json"
#define CANLOGMANAGER_CLEAR_ALL_LOGS 0U
#define PREALLOCATE_LOG_FILES        1U

#define CLM_E_OK          ((ClmErrorType)0U)
#define CLM_E_NOT_OK      ((ClmErrorType)1U)
#define CLM_E_BATCH_LIMIT ((ClmErrorType)2U)
#define CLM_E_BLOCK_READY ((ClmErrorType)3U)

FRESULT appCanLogHandlerInit(CanLogControlDataType *data);
void appCanLogSetFileConfig(uint32_t log_file_size, uint32_t log_file_count);
void appCanLogSetClusterSize(uint32_t cluster_size);
uint32_t appCanLogGetLogFileSize(void);
uint32_t appCanLogGetLogFileCount(void);
uint32_t appCanLogGetClusterSize(void);
uint32_t appCanLogGetMaxLogIndex(void);

/**
 * @brief Set runtime parameters for the CAN log manager.
 *
 * Currently supported parameters are CAN1/CAN2 baud rates.
 *
 * @param id Parameter selector (e.g. CLM_PARAMETER_ID_CAN1_BAUDRATE).
 * @param value New value for the selected parameter.
 * @return CAN_LOG_OK on success, CAN_LOG_ERR_INVALID_PARAM for unknown ids.
 *
 * @note Valid baudrates are 250000, 500000, and 1000000 (bit/s).
 */
CanLogResult appCanLogSetParam(ClmParameterIdType id, uint32_t value);
ClmErrorType appCanLogHandlerPoll(CanLogControlDataType *data);
void appCanLogHandlerDeInit(CanLogControlDataType *data);

/* Initialize the global control data */
CanLogControlDataType *
CanLogHandler_Init(uint8_t *mount_res, bool *run, bool *commit);

/* Hooks */
void CanLogFileManager_ErrorHandler(void);
