#include "FileHandler.h"

static FATFS FatFs;		/* FatFs work area needed for each volume */

FRESULT FatFS_SD_Mount()
{  
  return f_mount(&FatFs, "", 0U);		/* Give a work area to the default drive */
}

FRESULT FatFS_SD_Unmount()
{  
  return f_mount(NULL, "", 0U);		/* Give a work area to the default drive */
}

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name)
{  
  FRESULT fr;

  dev->fflags = FA_OPEN_APPEND | FA_WRITE;
  fr = f_open(&dev->file, name, dev->fflags);

  return fr;
}

FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name)
{  
  FRESULT fr;

  dev->fflags = FA_CREATE_ALWAYS | FA_WRITE;
  fr = f_open(&dev->file, name, dev->fflags);

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
    uint32_t FileSize = 0U;

    if ((dev->fflags & FA_CREATE_ALWAYS) == 0)  // Only seek if we did NOT truncate the file
    {
        FileSize = f_size(&dev->file);
    }
    else
    {
        FileSize = 0U;
    }

    if (FR_OK == f_lseek(&dev->file, FileSize))
    {
        /* write to the start of the file */
        res = f_write(&dev->file, content, len, &BytesWritten);
    }

    if ( 0 == res && len == BytesWritten )
        res = FR_OK;
    else
        res = FR_DISK_ERR;

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
    fmt_opt.au_size = cluster_size;   // 32KB clusters ← KEY SETTING

    res = f_mkfs(Dir, &fmt_opt, work_buffer, (UINT)sizeof(work_buffer));

    return res;
}

