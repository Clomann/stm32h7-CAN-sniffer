#pragma once

#include <stdint.h>

#include "ff.h"
#include "MmcAdapter.h"

typedef struct {
    uint32_t baudrate;
    /* CAN operatig mode (1: normal, 2: listen only)*/
    uint8_t mode; 
    uint8_t ip[4U];
} AppConfigType;

typedef struct {
    uint32_t readTargetSize;
    FIL file;
} FatFsDeviceType;

int FileHandler_ParseConfig(char *buff, uint32_t len, AppConfigType *cfg);

int FatFS_SD_LoadConfig(FatFsDeviceType *dev, char *data, uint32_t *len);

FRESULT FatFS_SD_Mount(void);
FRESULT FatFS_SD_Unmount(void);
FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_Flush(FatFsDeviceType *dev);
FRESULT FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *content, const uint32_t len);
FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t len);
FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size);
FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev);