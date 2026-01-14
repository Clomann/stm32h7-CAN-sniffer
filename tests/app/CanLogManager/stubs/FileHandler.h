#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "ff.h"
#include "FileHandlerTypes.h"

#define FILEHANDLER_PARTITION_NO   ""
#define FILEHANDLER_FORMATTING_REQUEST_FILENAME "formatting_requested.txt"

#define FATFS_SEEK_ON_WRITE_APPEND  0
#define FATFS_TRUNCATE_ON_SEEK_FAIL 0

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_Flush(FatFsDeviceType *dev);
FRESULT FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *content, const uint32_t len);
FRESULT FatFS_SD_WriteBeginningOfFile(
    FatFsDeviceType *dev,
    const char *content,
    const uint32_t len);
FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t len);
FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size);
FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size);
FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev);
FRESULT FatFS_SD_Formatting_Request(
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
);
FRESULT FatFS_SD_Format_Fat32(uint32_t cluster_size);

FRESULT FatFS_SD_FileIterator_Open(FatFS_FileIterator *it, const char *dirPath, const char *prefix);
FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **outInfo);
FRESULT FatFS_SD_FileIterator_Close(FatFS_FileIterator *it);

int FileHandler_GetValue(
    char *buff,
    uint32_t buffLen,
    char const *key,
    uint32_t keyLen,
    char **value,
    size_t *valLen);
int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base);

/* Helpers for tests */
void reset_filehandler_stubs(void);
void set_file_open_result(FRESULT result);
void set_file_size(uint32_t size);
bool get_file_closed(void);
bool get_file_open_failed(void);
