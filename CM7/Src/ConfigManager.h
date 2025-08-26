#pragma once

#include <stdint.h>

#include "FileHandler.h"

typedef struct ConfigFileManagerType ConfigFileManagerType;

typedef enum {
    CONFIG_SUCCESS = 0,
    CONFIG_ERROR_MOUNT_FAILED,
    CONFIG_ERROR_FILE_OPEN,
    CONFIG_ERROR_FILE_READ,
    CONFIG_ERROR_FILE_WRITE,
    CONFIG_ERROR_INVALID_PARAM,
    CONFIG_ERROR_NOT_INITIALIZED
} ConfigResult;

ConfigFileManagerType *ConfigManager_GetControlData(void);
ConfigFileManagerType * ConfigManager_Init(uint8_t *mount_res);
uint8_t ConfigManager_Load(ConfigFileManagerType *mgr);
uint8_t ConfigManager_Apply(ConfigFileManagerType *mgr);

FatFsDeviceType *ConfigManager_GetFile(ConfigFileManagerType *mng);
uint8_t ConfigManager_GetOpenRes(ConfigFileManagerType *mng);
void appConfigHandlerInit(ConfigFileManagerType *mng);