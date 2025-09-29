#pragma once

#include "FileHandlerTypes.h"
#include "MmcAdapter.h"

#define FILEHANDLER_FORMATTING_REQUEST_FILENAME "formatting_requested.txt"

FRESULT FatFS_SD_Mount(void);
FRESULT FatFS_SD_Unmount(void);
FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_Flush(FatFsDeviceType *dev);
FRESULT FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *content, const uint32_t len);
FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t len);
FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size);
FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size);
FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev);
FRESULT FatFS_SD_Formatting_Request(void);
FRESULT FatFS_SD_Format_Fat32(uint32_t cluster_size);

FRESULT FatFS_SD_FileIterator_Open(FatFS_FileIterator *it, const char *dirPath, const char *prefix);
FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **outInfo);
FRESULT FatFS_SD_FileIterator_Close(FatFS_FileIterator *it);