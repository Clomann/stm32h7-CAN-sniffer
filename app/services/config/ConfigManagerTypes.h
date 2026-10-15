#pragma once

typedef struct ConfigManager ConfigManagerType;

typedef enum
{
    CONFIG_OK = 0,
    CONFIG_NOT_OK,
    CONFIG_ERROR_MOUNT_FAILED,
    CONFIG_ERROR_FILE_OPEN,
    CONFIG_ERROR_FILE_READ,
    CONFIG_ERROR_FILE_WRITE,
    CONFIG_ERROR_INVALID_PARAM,
    CONFIG_ERROR_NOT_INITIALIZED,
    CONFIG_ERROR_PARSE_FAILED
} ConfigResultType;

typedef struct
{
    char filename[256];
    uint32_t size;
    uint32_t lastModified;
    bool isOpen;
    bool needsSync;
} ConfigFileInfoType;
