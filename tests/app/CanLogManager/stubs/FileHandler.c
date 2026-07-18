
#include "FileHandler.h"
#include "CanLogBuffer.h"
#include "test_filehandler_state.h"
#include "core_json.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

static FRESULT mock_file_open_result        = FR_OK;
static uint32_t mock_file_size              = 1000;
static bool mock_file_closed                = false;
static bool mock_file_open_failed           = false;
static uint64_t mock_total_bytes_written    = 0;
static uint64_t mock_total_frames_written   = 0;
static uint32_t mock_last_block_frame_count = 0;
static uint32_t mock_write_call_count       = 0;
static FRESULT mock_next_write_result       = FR_OK;
static FRESULT mock_next_flush_result       = FR_OK;

static const char mock_meta_filename[] = "/logs_meta.json";
static char mock_meta_file_content[128];
static uint32_t mock_meta_file_len    = 0;
static bool mock_meta_file_present    = false;
static bool mock_current_is_meta      = false;
static FatFsDeviceType *mock_meta_dev = NULL;

void reset_filehandler_stubs(void)
{
    mock_file_open_result       = FR_OK;
    mock_file_size              = 1000;
    mock_file_closed            = false;
    mock_file_open_failed       = false;
    mock_total_bytes_written    = 0;
    mock_total_frames_written   = 0;
    mock_last_block_frame_count = 0;
    mock_write_call_count       = 0;
    mock_next_write_result      = FR_OK;
    mock_next_flush_result      = FR_OK;
    mock_meta_file_len          = 0;
    mock_meta_file_present      = false;
    mock_current_is_meta        = false;
    mock_meta_dev               = NULL;
    mock_meta_file_content[0]   = '\0';
}

void set_file_open_result(FRESULT result)
{
    mock_file_open_result = result;
}

void set_file_size(uint32_t size)
{
    mock_file_size = size;
}

void set_next_write_result(FRESULT result)
{
    mock_next_write_result = result;
}

void set_next_flush_result(FRESULT result)
{
    mock_next_flush_result = result;
}

bool get_file_closed(void)
{
    return mock_file_closed;
}

bool get_file_open_failed(void)
{
    return mock_file_open_failed;
}

void test_filehandler_set_meta_content(const char *data, uint32_t len)
{
    if (data == NULL)
    {
        test_filehandler_clear_meta_content();
        return;
    }

    if (len >= sizeof(mock_meta_file_content))
    {
        len = (uint32_t)(sizeof(mock_meta_file_content) - 1U);
    }

    memcpy(mock_meta_file_content, data, len);
    mock_meta_file_content[len] = '\0';
    mock_meta_file_len          = len;
    mock_meta_file_present      = true;
}

void test_filehandler_clear_meta_content(void)
{
    mock_meta_file_len        = 0;
    mock_meta_file_present    = false;
    mock_meta_file_content[0] = '\0';
}

const char *test_filehandler_get_meta_content(uint32_t *len)
{
    if (len)
    {
        *len = mock_meta_file_len;
    }
    return mock_meta_file_content;
}

bool test_filehandler_meta_exists(void)
{
    return mock_meta_file_present;
}

uint32_t test_filehandler_meta_size(void)
{
    return mock_meta_file_len;
}

uint64_t get_total_bytes_written(void)
{
    return mock_total_bytes_written;
}

uint64_t get_total_frames_written(void)
{
    return mock_total_frames_written;
}

uint32_t get_last_block_frame_count(void)
{
    return mock_last_block_frame_count;
}

uint32_t get_write_call_count(void)
{
    return mock_write_call_count;
}

static uint32_t test_count_valid_entries(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length < sizeof(CanLogBlockHeaderType))
    {
        return 0U;
    }

    const CanLogBlockHeaderType *header = (const CanLogBlockHeaderType *)data;
    if ((header->version != CANLOG_VERSION)
        || (header->block_size != BLOCK_SIZE)
        || (header->header_size < sizeof(CanLogBlockHeaderType)))
    {
        return 0U;
    }

    uint32_t offset = header->header_size;
    uint32_t count  = 0U;

    while ((offset + sizeof(CanLogEntryHeaderType)) <= length)
    {
        const CanLogEntryHeaderType *entry =
            (const CanLogEntryHeaderType *)(data + offset);

        if (entry->type == CLB_ENTRY_TYPE_NONE)
        {
            break;
        }

        if ((entry->type != CLB_ENTRY_TYPE_FRAME)
            && (entry->type != CLB_ENTRY_TYPE_SYNC)
            && (entry->type != CLB_ENTRY_TYPE_MARKER))
        {
            break;
        }

        if ((entry->total_len < sizeof(CanLogEntryHeaderType))
            || ((offset + entry->total_len) > length))
        {
            break;
        }

        count++;
        offset += entry->total_len;
    }

    return count;
}

FRESULT FatFS_SD_OpenFileForWrite(FatFsDeviceType *dev, const char *name)
{
    (void)dev;
    (void)name;
    mock_current_is_meta = false;
    if (dev)
    {
        mock_meta_dev = NULL;
    }

    if (mock_file_open_result != FR_OK)
    {
        mock_file_open_failed = true;
    }
    return mock_file_open_result;
}

FRESULT FatFS_SD_OpenFileForRead(FatFsDeviceType *dev, const char *name)
{
    mock_current_is_meta =
        (name != NULL) && (strcmp(name, mock_meta_filename) == 0);
    if (mock_current_is_meta && !mock_meta_file_present)
    {
        return FR_NO_FILE;
    }
    if (mock_current_is_meta)
    {
        mock_meta_dev = dev;
    }
    else
    {
        mock_meta_dev = NULL;
    }
    return mock_file_open_result;
}

FRESULT FatFS_SD_GetBufferedFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    (void)dev;

    if (size)
    {
        *size = (dev == mock_meta_dev) ? mock_meta_file_len : mock_file_size;
    }
    return FR_OK;
}

FRESULT FatFS_SD_GetFileSize(FatFsDeviceType *dev, uint32_t *size)
{
    if (size)
    {
        *size = (dev == mock_meta_dev) ? mock_meta_file_len : mock_file_size;
    }
    return FR_OK;
}

FRESULT FatFS_SD_CloseFile(FatFsDeviceType *dev)
{
    (void)dev;

    mock_file_closed = true;
    if (dev == mock_meta_dev)
    {
        mock_meta_dev = NULL;
    }
    mock_current_is_meta = false;
    return FR_OK;
}

FRESULT
FatFS_SD_WriteFile(FatFsDeviceType *dev, const char *data, uint32_t length)
{
    (void)dev;

    if (dev == mock_meta_dev)
    {
        if (data == NULL)
        {
            return FR_INVALID_PARAMETER;
        }

        if (length >= sizeof(mock_meta_file_content))
        {
            length = (uint32_t)(sizeof(mock_meta_file_content) - 1U);
        }

        memcpy(mock_meta_file_content, data, length);
        mock_meta_file_content[length] = '\0';
        mock_meta_file_len             = length;
        mock_meta_file_present         = true;
        return FR_OK;
    }

    if (mock_next_write_result != FR_OK)
    {
        FRESULT result         = mock_next_write_result;
        mock_next_write_result = FR_OK;
        mock_write_call_count++;
        return result;
    }

    if ((data != NULL) && (length > 0U))
    {
        mock_total_bytes_written += length;
        mock_write_call_count++;
        mock_file_size += length;

        if (length >= sizeof(CanLogBlockHeaderType))
        {
            uint32_t count =
                test_count_valid_entries((const uint8_t *)data, length);
            if (count > 0U)
            {
                mock_last_block_frame_count = count;
                mock_total_frames_written += count;
            }
        }
    }

    return FR_OK;
}

FRESULT FatFS_SD_ReadFile(FatFsDeviceType *dev, char *data, uint32_t length)
{
    (void)dev;
    if (data && length > 0)
    {
        if (dev == mock_meta_dev)
        {
            uint32_t to_copy =
                (length < mock_meta_file_len) ? length : mock_meta_file_len;
            memcpy(data, mock_meta_file_content, to_copy);
        }
        else
        {
            memset(data, 0, length);
        }
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

    if (mock_next_flush_result != FR_OK)
    {
        FRESULT result         = mock_next_flush_result;
        mock_next_flush_result = FR_OK;
        return result;
    }

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
    mock_current_is_meta =
        (name != NULL) && (strcmp(name, mock_meta_filename) == 0);
    if (mock_current_is_meta)
    {
        mock_meta_file_len        = 0;
        mock_meta_file_present    = true;
        mock_meta_file_content[0] = '\0';
        mock_meta_dev             = dev;
    }
    else
    {
        mock_meta_dev = NULL;
    }
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
    JSONStatus_t result;

    if (valLen)
    {
        *valLen = 0U;
    }

    result = JSON_Search(buff, buffLen, key, keyLen, value, valLen);
    return result;
}

int FileHandler_ConvertToInteger(char *data, uint32_t *val, uint8_t base)
{
    char *endptr = NULL;
    unsigned long result;

    if ((data == NULL) || (val == NULL))
    {
        return 1;
    }

    result = strtoul(data, &endptr, base);
    if (endptr == data)
    {
        return 1;
    }

    *val = (uint32_t)result;
    return 0;
}
