#include <string.h>
#include <stdlib.h>

#include "ConfigManager.h"

__attribute__((weak)) int ConfigManager_SerializeHook(
    const void *config,
    char *buffer,
    uint32_t maxLength,
    uint32_t *length
)
{
    (void)config;
    (void)buffer;
    (void)maxLength;
    (void)length;

    return -1;
}

__attribute__((weak)) int
ConfigManager_DeserializeHook(const char *buffer, uint32_t length, void *config)
{
    (void)buffer;
    (void)length;
    (void)config;

    return -1;
}

ConfigResultType ConfigManager_LoadRaw(
    ConfigManagerType *manager,
    char *buffer,
    uint32_t bufferSize,
    uint32_t *bytesRead
);

ConfigResultType ConfigManager_SaveRaw(
    ConfigManagerType *manager,
    const char *data,
    uint32_t dataSize
);

ConfigResultType ConfigManager_Flush(ConfigManagerType *manager);

static ConfigResultType validateManager(const ConfigManagerType *manager);
static ConfigResultType openForRead(ConfigManagerType *manager);
static ConfigResultType openForWrite(ConfigManagerType *manager);
static void closeFile(ConfigManagerType *manager);

void ConfigManager_Init(
    ConfigManagerType *manager,
    const char *filename,
    uint8_t *mountResult
)
{
    if (!manager || !mountResult)
    {
        return;
    }

    memset(manager, 0, sizeof(ConfigManagerType));

    if (filename && strlen(filename) <= MAX_FILENAME_LENGTH)
    {
        strncpy(manager->filename, filename, MAX_FILENAME_LENGTH);
        manager->filename[MAX_FILENAME_LENGTH] = '\0';
    }
    else
    {
        strcpy(manager->filename, DEFAULT_CONFIG_FILENAME);
    }

    manager->mountRes    = mountResult;
    manager->initialized = false;
    manager->fileOpen    = false;
    manager->needsSync   = false;
    manager->lastError   = CONFIG_OK;
}

void ConfigManager_DeInit(ConfigManagerType *manager)
{
    if (!manager)
    {
        return;
    }

    if (manager->fileOpen)
    {
        closeFile(manager);
    }

    memset(manager, 0, sizeof(ConfigManagerType));
}

ConfigResultType ConfigManager_Initialize(ConfigManagerType *manager)
{
    ConfigResultType result = validateManager(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    if (*manager->mountRes != RES_OK)
    {
        manager->lastError = CONFIG_ERROR_MOUNT_FAILED;
        return CONFIG_ERROR_MOUNT_FAILED;
    }

    manager->initialized = true;
    manager->lastError   = CONFIG_OK;
    return CONFIG_OK;
}

ConfigResultType ConfigManager_LoadRaw(
    ConfigManagerType *manager,
    char *buffer,
    uint32_t bufferSize,
    uint32_t *bytesRead
)
{
    ConfigResultType result = validateManager(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    if (!buffer || bufferSize == 0)
    {
        manager->lastError = CONFIG_ERROR_INVALID_PARAM;
        return CONFIG_ERROR_INVALID_PARAM;
    }

    if (!manager->initialized)
    {
        manager->lastError = CONFIG_ERROR_NOT_INITIALIZED;
        return CONFIG_ERROR_NOT_INITIALIZED;
    }

    // Open file for reading
    result = openForRead(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    // Get file size
    uint32_t fileSize;
    FRESULT fr = FatFS_SD_GetFileSize(&manager->writeFileDevice, &fileSize);
    if (fr != FR_OK)
    {
        closeFile(manager);
        manager->lastError = CONFIG_ERROR_FILE_READ;
        return CONFIG_ERROR_FILE_READ;
    }

    // Read data
    uint32_t readSize = (fileSize < bufferSize) ? fileSize : bufferSize;
    fr = FatFS_SD_ReadFile(&manager->writeFileDevice, buffer, readSize);

    closeFile(manager);

    if (fr != FR_OK)
    {
        manager->lastError = CONFIG_ERROR_FILE_READ;
        return CONFIG_ERROR_FILE_READ;
    }

    if (bytesRead)
    {
        *bytesRead = readSize;
    }

    manager->fileSize  = fileSize;
    manager->lastError = CONFIG_OK;
    return CONFIG_OK;
}

ConfigResultType ConfigManager_SaveRaw(
    ConfigManagerType *manager,
    const char *data,
    uint32_t dataSize
)
{
    ConfigResultType result = validateManager(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    if (!data || dataSize == 0)
    {
        manager->lastError = CONFIG_ERROR_INVALID_PARAM;
        return CONFIG_ERROR_INVALID_PARAM;
    }

    if (!manager->initialized)
    {
        manager->lastError = CONFIG_ERROR_NOT_INITIALIZED;
        return CONFIG_ERROR_NOT_INITIALIZED;
    }

    // Open file for writing (overwrite)
    result = openForWrite(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    // Write data
    FRESULT fr = FatFS_SD_WriteFile(&manager->writeFileDevice, data, dataSize);
    if (fr != FR_OK)
    {
        closeFile(manager);
        manager->lastError = CONFIG_ERROR_FILE_WRITE;
        return CONFIG_ERROR_FILE_WRITE;
    }

    // Update file size and mark as needing sync
    manager->fileSize  = dataSize;
    manager->needsSync = true;

    manager->lastError = CONFIG_OK;
    return CONFIG_OK;
}

ConfigResultType ConfigManager_Flush(ConfigManagerType *manager)
{
    ConfigResultType result = validateManager(manager);
    if (result != CONFIG_OK)
    {
        return result;
    }

    if (!manager->fileOpen || !manager->needsSync)
    {
        return CONFIG_OK;
    }

    FRESULT fr = FatFS_SD_Flush(&manager->writeFileDevice);
    closeFile(manager);

    if (fr != FR_OK)
    {
        manager->lastError = CONFIG_ERROR_FILE_WRITE;
        return CONFIG_ERROR_FILE_WRITE;
    }

    manager->needsSync = false;
    manager->lastError = CONFIG_OK;
    return CONFIG_OK;
}

ConfigResultType
ConfigManager_LoadConfig(ConfigManagerType *manager, void *config)
{
    if (!config)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }

    uint32_t bytesRead;
    ConfigResultType result = ConfigManager_LoadRaw(
        manager,
        manager->buffer,
        CONFIG_BUFFER_SIZE,
        &bytesRead
    );

    if (result != CONFIG_OK)
    {
        return result;
    }

    int parseResult =
        ConfigManager_DeserializeHook(manager->buffer, bytesRead, config);
    if (parseResult != CONFIG_OK)
    {
        manager->lastError = CONFIG_ERROR_PARSE_FAILED;
        return CONFIG_ERROR_PARSE_FAILED;
    }

    return CONFIG_OK;
}

ConfigResultType
ConfigManager_SaveConfig(ConfigManagerType *manager, const void *config)
{
    if (!config)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }

    uint32_t serializedLength;
    int result = ConfigManager_SerializeHook(
        config,
        manager->buffer,
        CONFIG_BUFFER_SIZE,
        &serializedLength
    );

    if (result != 0)
    {
        manager->lastError = CONFIG_ERROR_PARSE_FAILED;
        return CONFIG_ERROR_PARSE_FAILED;
    }

    return ConfigManager_SaveRaw(manager, manager->buffer, serializedLength);
}

ConfigResultType ConfigManager_UpdateConfig(
    ConfigManagerType *manager,
    const void *config,
    bool needsUpdate
)
{
    if (!config)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }

    if (needsUpdate)
    {
        ConfigResultType result = ConfigManager_SaveConfig(manager, config);
        if (result == CONFIG_OK)
        {
            result = ConfigManager_Flush(manager);
        }
        return result;
    }

    return CONFIG_OK;
}

bool ConfigManager_IsInitialized(const ConfigManagerType *manager)
{
    return manager && manager->initialized;
}

bool ConfigManager_IsFileOpen(const ConfigManagerType *manager)
{
    return manager && manager->fileOpen;
}

bool ConfigManager_NeedsSync(const ConfigManagerType *manager)
{
    return manager && manager->needsSync;
}

ConfigResultType ConfigManager_GetFileInfo(
    const ConfigManagerType *manager,
    ConfigFileInfoType *info
)
{
    ConfigResultType result = validateManager(manager);
    if (result != CONFIG_OK || !info)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }

    strcpy(info->filename, manager->filename);
    info->size         = manager->fileSize;
    info->lastModified = manager->lastModified;
    info->isOpen       = manager->fileOpen;
    info->needsSync    = manager->needsSync;

    return CONFIG_OK;
}

static ConfigResultType validateManager(const ConfigManagerType *manager)
{
    if (!manager)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }
    if (!manager->mountRes)
    {
        return CONFIG_ERROR_INVALID_PARAM;
    }
    return CONFIG_OK;
}

static ConfigResultType openForRead(ConfigManagerType *manager)
{
    FRESULT fr =
        FatFS_SD_OpenFileForRead(&manager->writeFileDevice, manager->filename);

    if (fr == FR_OK)
    {
        manager->fileOpen = true;
        return CONFIG_OK;
    }
    else
    {
        manager->fileOpen  = false;
        manager->lastError = CONFIG_ERROR_FILE_OPEN;
        return CONFIG_ERROR_FILE_OPEN;
    }
}

static ConfigResultType openForWrite(ConfigManagerType *manager)
{
    FRESULT fr = FatFS_SD_OpenFileForOverWrite(
        &manager->writeFileDevice,
        manager->filename
    );

    if (fr == FR_OK)
    {
        manager->fileOpen = true;
        return CONFIG_OK;
    }
    else
    {
        manager->fileOpen  = false;
        manager->lastError = CONFIG_ERROR_FILE_OPEN;
        return CONFIG_ERROR_FILE_OPEN;
    }
}

static void closeFile(ConfigManagerType *manager)
{
    if (manager && manager->fileOpen)
    {
        FatFS_SD_CloseFile(&manager->writeFileDevice);
        manager->fileOpen = false;
    }
}

