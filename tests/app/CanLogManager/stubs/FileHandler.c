
#include "FileHandler.h"
#include <string.h>

static FRESULT mock_file_open_result = FR_OK;
static uint32_t mock_file_size       = 1000;
static bool mock_file_closed         = false;
static bool mock_file_open_failed    = false;

void reset_filehandler_stubs(void)
{
    mock_file_open_result = FR_OK;
    mock_file_size        = 1000;
    mock_file_closed      = false;
    mock_file_open_failed = false;
}

void set_file_open_result(FRESULT result)
{
    mock_file_open_result = result;
}

void set_file_size(uint32_t size)
{
    mock_file_size = size;
}

bool get_file_closed(void)
{
    return mock_file_closed;
}

bool get_file_open_failed(void)
{
    return mock_file_open_failed;
}

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name)
{
    (void)dev;
    (void)name;

    if (mock_file_open_result != FR_OK)
    {
        mock_file_open_failed = true;
    }
    return mock_file_open_result;
}

FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name)
{
    (void)dev;
    (void)name;
    return mock_file_open_result;
}

FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    (void)dev;

    if (size)
    {
        *size = mock_file_size;
    }
    return FR_OK;
}

FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    (void)dev;
    if (size)
    {
        *size = mock_file_size;
    }
    return FR_OK;
}

FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev)
{
    (void)dev;

    mock_file_closed = true;
    return FR_OK;
}

FRESULT
FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *data, uint32_t length)
{
    (void)dev;
    (void)data;
    (void)length;

    return FR_OK;
}

FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t length)
{
    (void)dev;
    if (data && length > 0)
    {
        memset(data, 0, length);
    }
    return FR_OK;
}

FRESULT FatFS_SD_Formatting_Request(
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    (void)cluster_size;
    (void)log_file_size;
    (void)log_file_count;

    return FR_OK;
}

FRESULT FatFS_SD_Flush(FatFsDeviceType *dev)
{
    (void)dev;

    return FR_OK;
}

FRESULT FatFS_SD_FileIterator_Open(
    FatFS_FileIterator *it,
    const char *path,
    const char *prefix
)
{
    (void)it;
    (void)path;
    (void)prefix;

    return FR_NO_FILE;
}

FRESULT FatFS_SD_FileIterator_Next(FatFS_FileIterator *it, FILINFO **fno)
{
    (void)it;
    (void)fno;

    return FR_NO_FILE;
}

FRESULT FatFS_SD_FileIterator_Close(FatFS_FileIterator *it)
{
    (void)it;

    return FR_OK;
}

FRESULT FatFS_SD_OpenFileForOverWrite(FatFsDeviceType *dev, const char *name)
{
    (void)dev;
    (void)name;
    return FR_OK;
}

FRESULT FatFS_SD_Format_Fat32(uint32_t cluster_size)
{
    (void)cluster_size;
    return FR_OK;
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
    (void)buff;
    (void)buffLen;
    (void)key;
    (void)keyLen;
    if (value)
    {
        *value = NULL;
    }
    if (valLen)
    {
        *valLen = 0;
    }
    return 0;
}

int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base)
{
    (void)data;
    if (val)
    {
        *val = 0;
    }
    (void)base;
    return 0;
}
