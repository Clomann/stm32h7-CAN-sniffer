#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ErrorContext.h"
#include "FileHandler.h"
#include "ff.h"

#include "core_json.h"

#define FATFS_SD_PHYSICAL_DRIVE         0U
#define FATFS_SD_MBR_SIGNATURE_OFFSET   510U
#define FATFS_SD_MBR_SIGNATURE_FIRST    0x55U
#define FATFS_SD_MBR_SIGNATURE_SECOND   0xAAU
#define FATFS_SD_WHOLE_DRIVE_PERCENTAGE 100U
#define FATFS_SD_IO_OK                  0U

static FATFS FatFs; /* FatFs work area needed for each volume */
static ErrorContextType ErrorContext;

static void FatFS_SD_SetErrorContext(
    ErrorIdType id,
    ErrorCodeType code,
    uint16_t line,
    const char *function,
    uint32_t arg0,
    uint32_t arg1
)
{
    memset(&ErrorContext, 0, sizeof(ErrorContext));
    ErrorContext.id   = id;
    ErrorContext.code = code;
    ErrorContext.arg0 = arg0;
    ErrorContext.arg1 = arg1;
    ErrorContext.line = line;
    snprintf(ErrorContext.file, sizeof(ErrorContext.file), "%s", __FILE_NAME__);
    snprintf(
        ErrorContext.function,
        sizeof(ErrorContext.function),
        "%s",
        function
    );
}

#define FATFS_SD_SET_ERROR_CONTEXT(id, code, arg0, arg1)                       \
    FatFS_SD_SetErrorContext(                                                  \
        (id),                                                                  \
        (ErrorCodeType)(code),                                                 \
        (uint16_t)__LINE__,                                                    \
        __func__,                                                              \
        (uint32_t)(arg0),                                                      \
        (uint32_t)(arg1)                                                       \
    )

static bool FatFS_SD_MbrSignatureIsValid(const BYTE *sector)
{
    return (sector[FATFS_SD_MBR_SIGNATURE_OFFSET]
            == FATFS_SD_MBR_SIGNATURE_FIRST)
           && (sector[FATFS_SD_MBR_SIGNATURE_OFFSET + 1U]
               == FATFS_SD_MBR_SIGNATURE_SECOND);
}

static FRESULT FatFS_SD_RepartitionIfMbrIsMissing(BYTE *work_buffer)
{
    const LBA_t partition_table[] = {FATFS_SD_WHOLE_DRIVE_PERCENTAGE, 0U};

    if (MMCAdapter_read(work_buffer, 0U, 1U) != FATFS_SD_IO_OK)
    {
        return FR_DISK_ERR;
    }

    if (FatFS_SD_MbrSignatureIsValid(work_buffer))
    {
        return FR_MKFS_ABORTED;
    }

    return f_fdisk(FATFS_SD_PHYSICAL_DRIVE, partition_table, work_buffer);
}

/**
 * @brief Hook called before a FatFS write to allow measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void FileHandler_InstrumentationWriteStartHook(void)
{
}

/**
 * @brief Hook called after a FatFS write to allow measurement instrumentation.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void FileHandler_InstrumentationWriteEndHook(void)
{
}

__attribute__((weak)) void FileHandler_ErrorHandler(ErrorContextType *context)
{
    (void)context;

    __asm volatile("nop");
}

FRESULT FatFS_SD_Mount(void)
{
    FRESULT res;
    res = f_mount(&FatFs, FILEHANDLER_PARTITION_NO, 1U);

    if (FR_OK != res)
    {
        res = f_mount(&FatFs, FILEHANDLER_PARTITION_NO, 1U);
    }

    return res;
}

FRESULT FatFS_SD_Unmount(void)
{
    return f_mount(NULL, FILEHANDLER_PARTITION_NO, 0U);
}

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name)
{
    FRESULT fr;

    dev->fflags = FA_OPEN_APPEND | FA_WRITE;
    fr          = f_open(&dev->file, name, (BYTE)dev->fflags);

#if FATFS_SEEK_ON_WRITE_APPEND
    if (FR_OK == fr)
    {
        fr = f_lseek(&dev->file, dev->writeIndex);
    }

    if (fr != FR_OK)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_OPEN_WRITE_SEEK,
            fr,
            dev->writeIndex,
            dev->fflags
        );
        fr = f_truncate(&dev->file);

        if (fr == FR_OK)
        {
            // Reset to last valid write pointer and keep allocation
            fr = f_tell(&dev->file);
        }
        else
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_OPEN_WRITE_TRUNC,
                fr,
                dev->writeIndex,
                dev->fflags
            );
        }
    }
#endif

    if (FR_OK != fr)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_OPEN_WRITE,
            fr,
            dev->writeIndex,
            dev->fflags
        );
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return fr;
}

FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name)
{
    FRESULT fr;

    dev->fflags     = FA_WRITE;
    dev->writeIndex = 0;

    fr = f_open(&dev->file, name, (BYTE)dev->fflags);

    if (FR_OK != fr)
    {
        FRESULT open_fr = fr;

        fr = f_open(&dev->file, name, (BYTE)(dev->fflags | FA_CREATE_ALWAYS));
        if (FR_OK != fr)
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_OPEN_OVERWRITE,
                fr,
                (uint32_t)open_fr,
                dev->fflags | FA_CREATE_ALWAYS
            );
        }
    }
    else
    {
    }

    if (fr == FR_OK)
    {
        // Reset to beginning but keep allocation
        fr = f_lseek(&dev->file, 0);
        if (FR_OK != fr)
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_OVERWRITE_SEEK,
                fr,
                0U,
                dev->fflags
            );
        }
    }

#if FATFS_TRUNCATE_ON_SEEK_FAIL
    if (fr != FR_OK)
    {
        fr = f_truncate(&dev->file);
        if (fr == FR_OK)
        {
            // Reset to beginning but keep allocation
            fr = f_lseek(&dev->file, 0);
        }
        else
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_OVERWRITE_TRUNC,
                fr,
                0U,
                dev->fflags
            );
        }
    }
#endif

    if (FR_OK != fr)
    {
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return fr;
}

FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name)
{
    FRESULT fr;

    dev->fflags = FA_READ;
    fr = f_open(&dev->file, name, (BYTE)dev->fflags); /* Create a file */

    return fr;
}

FRESULT FatFS_SD_Flush(FatFsDeviceType *dev)
{
    FRESULT res;

    res = f_sync(&dev->file);

    if (FR_OK != res)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_F_SYNC,
            res,
            f_tell(&dev->file),
            dev->fflags
        );
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return res;
}

_Bool FatFS_SD_SyncPending(FatFsDeviceType *dev)
{
    const FATFS *fs;

    if (NULL == dev)
    {
        return 0;
    }

    fs = dev->file.obj.fs;

    /* wflag / fsi_flag b0 are set only when a cluster is (de)allocated, not
     * by an in-place write. FA_DIRTY is skipped: writes are sector-aligned. */
    return (NULL != fs)
           && ((0U != fs->wflag) || (0U != (fs->fsi_flag & 0x01U)));
}

FRESULT FatFS_SD_WriteFile(
    FatFsDeviceType *dev,
    const char *content,
    const uint32_t len
)
{
    FRESULT res       = FR_OK;
    UINT BytesWritten = 0U;
    uint8_t Truncate;
    uint32_t FileSize = 0U;

    Truncate = (dev->fflags & FA_CREATE_ALWAYS);

    if (Truncate > 0) // Only seek if we did NOT truncate the file
    {
        FileSize = 0U;
    }
    else
    {
        FileSize = f_tell(&dev->file);
    }

    res = f_lseek(&dev->file, FileSize);

    if (FR_OK != res)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_WRITE_LSEEK,
            res,
            FileSize,
            len
        );

        // File corrupted - truncate to known good size and continue
        res = f_truncate(&dev->file);

        if (FR_OK == res)
        {
            res = f_lseek(&dev->file, f_tell(&dev->file));
            if (FR_OK != res)
            {
                FATFS_SD_SET_ERROR_CONTEXT(
                    FILEHANDLER_ERROR_ID_WRITE_RESEEK,
                    res,
                    f_tell(&dev->file),
                    len
                );
            }
        }
        else
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_WRITE_TRUNCATE,
                res,
                FileSize,
                len
            );
        }
    }

    if (FR_OK == res)
    {
        FileHandler_InstrumentationWriteStartHook();
        res = f_write(&dev->file, content, len, &BytesWritten);
        if (FR_OK != res)
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_WRITE_F_WRITE,
                res,
                len,
                BytesWritten
            );
        }
        FileHandler_InstrumentationWriteEndHook();
    }

    if (FR_OK == res && BytesWritten != len)
    {
        res = FR_DISK_ERR; // Partial write = error
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_WRITE_PARTIAL,
            res,
            len,
            BytesWritten
        );
    }

    if (res != FR_OK)
    {
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return res;
}

FRESULT FatFS_SD_WriteBeginningOfFile(
    FatFsDeviceType *dev,
    const char *content,
    const uint32_t len
)
{
    FRESULT res       = FR_OK;
    UINT BytesWritten = 0U;

    res = f_lseek(&dev->file, 0);

    if (FR_OK != res)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_WRITE_BEGIN_SEEK,
            res,
            0U,
            len
        );

        // File corrupted - truncate to known good size and continue
        res = f_truncate(&dev->file);

        if (FR_OK == res)
        {
            res = f_lseek(&dev->file, 0);
            if (FR_OK != res)
            {
                FATFS_SD_SET_ERROR_CONTEXT(
                    FILEHANDLER_ERROR_ID_WRITE_BEGIN_SEEK,
                    res,
                    0U,
                    len
                );
            }
        }
        else
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_WRITE_BEGIN_TRUNC,
                res,
                0U,
                len
            );
        }
    }

    if (FR_OK == res)
    {
        res = f_write(&dev->file, content, len, &BytesWritten);
        if (FR_OK != res)
        {
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_WRITE_BEGIN_WRITE,
                res,
                len,
                BytesWritten
            );
        }
    }

    if (FR_OK == res)
    {
        if (BytesWritten != len)
        {
            res = FR_DISK_ERR; // Partial write = error
            FATFS_SD_SET_ERROR_CONTEXT(
                FILEHANDLER_ERROR_ID_WRITE_BEGIN_PART,
                res,
                len,
                BytesWritten
            );
        }
        else
        {
            res = f_lseek(&dev->file, 0);
            if (FR_OK != res)
            {
                FATFS_SD_SET_ERROR_CONTEXT(
                    FILEHANDLER_ERROR_ID_WRITE_BEGIN_RESET,
                    res,
                    0U,
                    len
                );
            }
        }
    }

    if (FR_OK != res)
    {
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return res;
}

FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t len)
{
    FRESULT fr;
    UINT BytesRead = 0U;

    fr = f_read(&dev->file, data, len, &BytesRead);

    if (FR_OK != fr)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_READ_F_READ,
            fr,
            len,
            BytesRead
        );
        FileHandler_ErrorHandler(&ErrorContext);
    }
    else if (BytesRead != len)
    {
        fr = FR_DISK_ERR;
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_READ_PARTIAL,
            fr,
            len,
            BytesRead
        );
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return fr;
}

FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev)
{
    FRESULT fr;

    if (NULL == dev->file.obj.fs)
    {
        return FR_OK;
    }

    dev->writeIndex = f_tell(&dev->file);

    fr = f_close(&dev->file); /* Close the file */
    if (FR_OK != fr)
    {
        FATFS_SD_SET_ERROR_CONTEXT(
            FILEHANDLER_ERROR_ID_CLOSE,
            fr,
            dev->writeIndex,
            dev->fflags
        );
        FileHandler_ErrorHandler(&ErrorContext);
    }

    return fr;
}

FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    FRESULT res;

    res = FR_OK;

    *size = f_size(&dev->file);
    return res; /* Close the file */
}

FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    FRESULT res;

    res = FR_OK;

    *size = f_tell(&dev->file);
    return res; /* Close the file */
}

FRESULT FatFS_SD_FileIterator_Open(
    FatFS_FileIterator *it,
    const char *dirPath,
    const char *prefix
)
{
    it->prefix    = prefix;
    it->prefixLen = strlen(prefix);
    return f_opendir(&it->dir, dirPath);
}

FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **outInfo)
{
    FRESULT res;

    while (1)
    {
        res = f_readdir(&it->dir, &it->fno);
        if (res != FR_OK)
        {
            return res; // end of dir or error
        }

        if (it->fno.fname[0] == 0)
        {
            return FR_NO_FILE; // Indicate end of directory
        }

        if (!(it->fno.fattrib & AM_DIR)
            && strncmp(it->fno.fname, it->prefix, it->prefixLen) == 0)
        {
            *outInfo = &it->fno;
            return FR_OK;
        }
    }
}

FRESULT FatFS_SD_FileIterator_Close(FatFS_FileIterator *it)
{
    return f_closedir(&it->dir);
}

FRESULT FatFS_SD_Formatting_Request(
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    FRESULT res;
    FatFsDeviceType File;
    const char FormatRequestFileName[] =
        FILEHANDLER_FORMATTING_REQUEST_FILENAME;
    char content[128];
    int length;

    res = FatFS_SD_OpenFileForOverWrite(&File, FormatRequestFileName);

    if (res != FR_OK)
    {
        return res;
    }

    length = snprintf(
        content,
        sizeof(content),
        "{\"cluster_size\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}",
        (unsigned long)cluster_size,
        (unsigned long)log_file_size,
        (unsigned long)log_file_count
    );

    if (length < 0 || (size_t)length >= sizeof(content))
    {
        (void)FatFS_SD_CloseFile(&File);
        return FR_INVALID_PARAMETER;
    }

    res = FatFS_SD_WriteFile(&File, content, (uint32_t)length);

    (void)FatFS_SD_CloseFile(&File);

    return res;
}

FRESULT FatFS_SD_Format_Fat32(uint32_t cluster_size)
{
    FRESULT res;
    const TCHAR Dir[] = "0:/";
    MKFS_PARM fmt_opt;
    BYTE work_buffer[FF_MAX_SS];

    fmt_opt.fmt     = FM_FAT32; // Force FAT32
    fmt_opt.n_fat   = 2; // Number of FAT copies (1 or 2)
    fmt_opt.align   = 0; // Alignment (0 = auto)
    fmt_opt.n_root  = 0; // Number of root entries (0 = auto for FAT32)
    fmt_opt.au_size = cluster_size;

    res = f_mkfs(Dir, &fmt_opt, work_buffer, (UINT)sizeof(work_buffer));

    if (FR_MKFS_ABORTED == res)
    {
        FRESULT repartition_res;

        repartition_res = FatFS_SD_RepartitionIfMbrIsMissing(work_buffer);

        if (FR_OK == repartition_res)
        {
            res = f_mkfs(Dir, &fmt_opt, work_buffer, (UINT)sizeof(work_buffer));
        }
        else if (FR_MKFS_ABORTED != repartition_res)
        {
            res = repartition_res;
        }
    }

    return res;
}

int FileHandler_GetValue(
    char *buff,
    uint32_t buffLen,
    char const *key,
    uint32_t keyLen,
    char **value,
    size_t *valLen
)
{
    JSONStatus_t result;

    *valLen = 0U;

    result = JSON_Search(buff, buffLen, key, keyLen, value, valLen);

    if (result == JSONSuccess)
    {
        // The pointer "value" will point to a location in the "buffer".
        char save = (*value)[*valLen];
        // After saving the character, set it to a null byte for printing.
        (*value)[*valLen] = '\0';

        // // Restore the original character.
        (*value)[*valLen] = save;
    }

    return result;
}

int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base)
{
    char *endptr;
    long value = strtol(data, &endptr, base);

    // Check if the conversion was successful
    if (*endptr != '\0')
    {
        // Handle conversion error: non-numeric characters were encountered
    }
    else
    {
        *val = (uint32_t)value;
    }

    return 0U;
}
