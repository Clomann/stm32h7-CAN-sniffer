#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "CanLogBuffer.h"
#include "CanLogManager.h"
#include "FileHandler.h"
#include "fs_custom.h"
#include "CommTypes.h"

#define RES_OK 0

#ifndef FF_DEFINED
typedef enum
{
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER
} FRESULT;

typedef struct
{
    uint32_t fsize;
    uint16_t fdate;
    uint16_t ftime;
    uint8_t fattrib;
    char fname[13];
} FILINFO;

typedef struct
{
    int dummy;
} FIL;

typedef struct
{
    int dummy;
} DIR;

#    define AM_DIR 0x10
#endif
