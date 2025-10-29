#include <stdlib.h>

#include "FileHandler.h"
#include "ff.h"

#include "core_json.h"

static FATFS FatFs;		/* FatFs work area needed for each volume */
static FATFS FatFs2;

FRESULT FatFS_SD_Mount()
{  
    FRESULT res;
    res =  f_mount(&FatFs, FILEHANDLER_PARTITION_NO, 1U);
    
    if (FR_OK == res)
    {
        res = f_mount(&FatFs, FILEHANDLER_PARTITION_NO, 1U);
    }

    return res;
}

FRESULT FatFS_SD_Unmount()
{  
  return f_mount(NULL, FILEHANDLER_PARTITION_NO, 1U);
}

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name)
{  
    FRESULT fr;
    
    dev->fflags = FA_OPEN_APPEND | FA_WRITE;
    fr = f_open(&dev->file, name, dev->fflags);
    
#if FATFS_SEEK_ON_WRITE_APPEND
    if (FR_OK == fr)
    {
        fr = f_lseek(&dev->file, dev->writeIndex);
    }
    
  if (fr != FR_OK)
  {
    fr = f_truncate(&dev->file);
    
    if (fr == FR_OK) 
    {
      // Reset to last valid write pointer and keep allocation
      fr = f_tell(&dev->file);
    }      
  }
#endif

  return fr;
}

FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name)
{  
    FRESULT fr;

    dev->fflags = FA_WRITE;
    dev->writeIndex = 0;
    
    fr = f_open(&dev->file, name, dev->fflags);

    if (FR_OK != fr)
    {
        fr = f_open(&dev->file, name, dev->fflags | FA_CREATE_ALWAYS);
    }
    else
    {
    }

    if (fr == FR_OK) 
    {
        // Reset to beginning but keep allocation
        fr = f_lseek(&dev->file, 0);
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
    }
#endif

    return fr;
}

FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name)
{  
  FRESULT fr;

  dev->fflags = FA_READ;
  fr = f_open(&dev->file, name, dev->fflags);	/* Create a file */

  return fr;
}

FRESULT FatFS_SD_Flush(FatFsDeviceType *dev)
{
    FRESULT res;

    res = f_sync(&dev->file);

    return res;
}

FRESULT FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *content, const uint32_t len)
{
    FRESULT res = FR_OK;
    UINT BytesWritten = 0U;
    uint8_t Truncate;
    uint32_t FileSize = 0U;
    
    Truncate = (dev->fflags & FA_CREATE_ALWAYS);

    if (Truncate > 0)  // Only seek if we did NOT truncate the file
    {
        FileSize = 0U;
    }
    else
    {
        FileSize = f_tell(&dev->file);
    }

    res = f_lseek(&dev->file, FileSize);

    if (FR_OK != res) {
        // File corrupted - truncate to known good size and continue
        res = f_truncate(&dev->file);
        
        if (FR_OK == res) 
        {
            res = f_lseek(&dev->file, f_tell(&dev->file));
        }
    }

    if (FR_OK == res)
    {
        res = f_write(&dev->file, content, len, &BytesWritten);
    }

    if (FR_OK == res && BytesWritten != len) {
        res = FR_DISK_ERR;  // Partial write = error
    }

    return res;
}

FRESULT FatFS_SD_WriteBeginningOfFile(
    FatFsDeviceType *dev, 
    const char *content, 
    const uint32_t len
)
{
    FRESULT res = FR_OK;
    UINT BytesWritten = 0U;

    res = f_lseek(&dev->file, 0);

    if (FR_OK != res) {
        // File corrupted - truncate to known good size and continue
        res = f_truncate(&dev->file);
        
        if (FR_OK == res) 
        {
            res = f_lseek(&dev->file, 0);
        }
    }

    if (FR_OK == res)
    {
        res = f_write(&dev->file, content, len, &BytesWritten);
    }

    if (FR_OK == res && BytesWritten != len) {
        res = FR_DISK_ERR;  // Partial write = error
    }
    else
    {
        res = f_lseek(&dev->file, 0);
    }

    return res;
}

FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t len)
{
	FRESULT fr;
    UINT BytesRead = 0U;

    fr = f_read(&dev->file, data, len, &BytesRead);

    if (BytesRead != len) fr = FR_DISK_ERR;

    return fr;
}

FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev)
{
    dev->writeIndex = f_tell(&dev->file);

    return f_close(&dev->file);							/* Close the file */
}

FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    FRESULT res;

    res = FR_OK;

    *size = f_size(&dev->file);
    return res;							/* Close the file */
}

FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    FRESULT res;

    res = FR_OK;

    *size = f_tell(&dev->file);
    return res;							/* Close the file */
}

FRESULT FatFS_SD_FileIterator_Open(FatFS_FileIterator *it, const char *dirPath, const char *prefix)
{
    it->prefix = prefix;
    it->prefixLen = strlen(prefix);
    return f_opendir(&it->dir, dirPath);
}

FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **outInfo)
{
    FRESULT res;

    while (1) {
        res = f_readdir(&it->dir, &it->fno);
        if (res != FR_OK)
            return res; // end of dir or error

        if (it->fno.fname[0] == 0)
            return FR_NO_FILE;  // Indicate end of directory

        if (!(it->fno.fattrib & AM_DIR) &&
            strncmp(it->fno.fname, it->prefix, it->prefixLen) == 0)
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

FRESULT FatFS_SD_Formatting_Request(void)
{
    FRESULT res;
    FatFsDeviceType File;
    const char FormatRequestFileName[] = FILEHANDLER_FORMATTING_REQUEST_FILENAME;

    FatFS_SD_OpenFileForOverWrite(&File, FormatRequestFileName);

    FatFS_SD_CloseFile(&File);

    return res;
}

FRESULT FatFS_SD_Format_Fat32(uint32_t cluster_size)
{
    FRESULT res;
    const TCHAR Dir[] = "0:/";
    MKFS_PARM fmt_opt;
    BYTE work_buffer[FF_MAX_SS];
    
    fmt_opt.fmt = FM_FAT32;           // Force FAT32
    fmt_opt.n_fat = 2;                // Number of FAT copies (1 or 2)
    fmt_opt.align = 0;                // Alignment (0 = auto)
    fmt_opt.n_root = 0;               // Number of root entries (0 = auto for FAT32)
    fmt_opt.au_size = cluster_size; 

    res = f_mkfs(Dir, &fmt_opt, work_buffer, (UINT)sizeof(work_buffer));

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
    
    result = JSON_Search( buff, buffLen, key, keyLen,
        value, valLen );
        
    if( result == JSONSuccess )
    {
        // The pointer "value" will point to a location in the "buffer".
        char save = (*value)[ *valLen ];
        // After saving the character, set it to a null byte for printing.
        (*value)[ *valLen ] = '\0';
        
        // // Restore the original character.
        (*value)[ *valLen ] = save;  
    }

    return result;
}

int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base)
{
    char *endptr;
    long value = strtol(data, &endptr, base);

    // Check if the conversion was successful
    if (*endptr != '\0') {
        // Handle conversion error: non-numeric characters were encountered
    } 
    else
    {
        *val = (uint32_t) value;
    }

    return 0U;
}
