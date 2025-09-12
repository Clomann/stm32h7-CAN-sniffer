#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "FileHandler.h"
#include "ConfigManagerTypes.h"

#define DEFAULT_CONFIG_FILENAME "CONF.TXT"
#define MAX_FILENAME_LENGTH     255
#define CONFIG_BUFFER_SIZE      512

struct ConfigManager
{
    char filename[MAX_FILENAME_LENGTH + 1];
    uint32_t fnamemaxlen;
    uint32_t fileSize;
    uint32_t timestamp;
    uint8_t openRes;
    FatFsDeviceType writeFileDevice;
    uint8_t *mountRes;
    uint32_t lastModified;
    bool initialized;
    bool fileOpen;
    bool needsSync;
    ConfigResultType lastError;
    char buffer[CONFIG_BUFFER_SIZE];
};

void ConfigManager_Init(
    ConfigManagerType *manager,
    const char *filename,
    uint8_t *mountResult
);

void ConfigManager_DeInit(ConfigManagerType *manager);

ConfigResultType ConfigManager_Initialize(ConfigManagerType *manager);

ConfigResultType
ConfigManager_LoadConfig(ConfigManagerType *manager, void *config);

ConfigResultType
ConfigManager_SaveConfig(ConfigManagerType *manager, const void *config);

ConfigResultType ConfigManager_UpdateConfig(
    ConfigManagerType *manager,
    const void *config,
    bool needsUpdate
);

bool ConfigManager_IsInitialized(const ConfigManagerType *manager);

bool ConfigManager_IsFileOpen(const ConfigManagerType *manager);

bool ConfigManager_NeedsSync(const ConfigManagerType *manager);

ConfigResultType ConfigManager_GetFileInfo(
    const ConfigManagerType *manager,
    ConfigFileInfoType *info
);

/* Hooks */

__attribute__((weak)) int ConfigManager_SerializeHook(
    const void *config,
    char *buffer,
    uint32_t maxLength,
    uint32_t *length
);

__attribute__((weak)) int ConfigManager_DeserializeHook(
    const char *buffer,
    uint32_t length,
    void *config
);
