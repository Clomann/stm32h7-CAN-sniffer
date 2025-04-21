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

  fr = f_open(&dev->file, name, FA_OPEN_APPEND | FA_WRITE );	/* Create a file */

  return fr;
}

FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name)
{  
  FRESULT fr;

  fr = f_open(&dev->file, name, FA_READ );	/* Create a file */

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
    FRESULT res;
    UINT BytesWritten;
    uint32_t FileSize = 0U;

    FileSize = f_size(&dev->file);
    f_lseek(&dev->file, FileSize);  // Move file pointer to the end
    f_write(&dev->file, content, len, &BytesWritten);

    if ( len == BytesWritten )
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