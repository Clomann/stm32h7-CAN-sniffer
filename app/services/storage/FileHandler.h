#pragma once

#include "ErrorContext.h"
#include "FileHandlerTypes.h"
#include "MmcAdapter.h"

#define FILEHANDLER_PARTITION_NO                ""
#define FILEHANDLER_FORMATTING_REQUEST_FILENAME "formatting_requested.txt"

#define FATFS_SEEK_ON_WRITE_APPEND  0
#define FATFS_TRUNCATE_ON_SEEK_FAIL 0

#define FILEHANDLER_ERROR_ID_OPEN_WRITE        0x46480001U
#define FILEHANDLER_ERROR_ID_OPEN_WRITE_SEEK   0x46480002U
#define FILEHANDLER_ERROR_ID_OPEN_WRITE_TRUNC  0x46480003U
#define FILEHANDLER_ERROR_ID_OPEN_OVERWRITE    0x46480004U
#define FILEHANDLER_ERROR_ID_OVERWRITE_SEEK    0x46480005U
#define FILEHANDLER_ERROR_ID_OVERWRITE_TRUNC   0x46480006U
#define FILEHANDLER_ERROR_ID_F_SYNC            0x46480007U
#define FILEHANDLER_ERROR_ID_WRITE_LSEEK       0x46480008U
#define FILEHANDLER_ERROR_ID_WRITE_TRUNCATE    0x46480009U
#define FILEHANDLER_ERROR_ID_WRITE_RESEEK      0x4648000AU
#define FILEHANDLER_ERROR_ID_WRITE_F_WRITE     0x4648000BU
#define FILEHANDLER_ERROR_ID_WRITE_PARTIAL     0x4648000CU
#define FILEHANDLER_ERROR_ID_WRITE_BEGIN_SEEK  0x4648000DU
#define FILEHANDLER_ERROR_ID_WRITE_BEGIN_TRUNC 0x4648000EU
#define FILEHANDLER_ERROR_ID_WRITE_BEGIN_WRITE 0x4648000FU
#define FILEHANDLER_ERROR_ID_WRITE_BEGIN_PART  0x46480010U
#define FILEHANDLER_ERROR_ID_WRITE_BEGIN_RESET 0x46480011U
#define FILEHANDLER_ERROR_ID_READ_F_READ       0x46480012U
#define FILEHANDLER_ERROR_ID_READ_PARTIAL      0x46480013U
#define FILEHANDLER_ERROR_ID_CLOSE             0x46480014U

FRESULT FatFS_SD_Mount(void);
FRESULT FatFS_SD_Unmount(void);
FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name);
FRESULT FatFS_SD_Flush(FatFsDeviceType *dev);

/* True when f_sync has FS metadata to persist (dirty FAT/dir window or
 * FSINFO change). False for an in-place write into a preallocated file. */
_Bool FatFS_SD_SyncPending(FatFsDeviceType *dev);

FRESULT FatFS_SD_WriteFile(
    FatFsDeviceType *dev,
    const char *content,
    const uint32_t len
);

FRESULT FatFS_SD_WriteBeginningOfFile(
    FatFsDeviceType *dev,
    const char *content,
    const uint32_t len
);

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

FRESULT FatFS_SD_FileIterator_Open(
    FatFS_FileIterator *it,
    const char *dirPath,
    const char *prefix
);
FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **outInfo);
FRESULT FatFS_SD_FileIterator_Close(FatFS_FileIterator *it);

int FileHandler_GetValue(
    char *buff,
    uint32_t buffLen,
    char const *key,
    uint32_t keyLen,
    char **value,
    size_t *valLen
);
int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base);

void FileHandler_ErrorHandler(ErrorContextType *context);

/* Weak instrumentation hooks, overridable by the application layer. */
void FileHandler_InstrumentationWriteStartHook(void);
void FileHandler_InstrumentationWriteEndHook(void);
