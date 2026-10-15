#include "ff.h"
#include "lwip/apps/fs.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fs_custom.h"
#include "fs_custom_internal.h"
#include "FileHandler.h"
#include "CanLogManager.h"

#if LWIP_HTTPD_CUSTOM_FILES

#define CANLOG_MAX_PATH_LENGTH 64U

#define CANLOG_META_DATA_STRING                                                \
    "{\"head\":%lu,\"tail\":%lu,\"capacity\":%lu,\"latest\":\"CAN.LOG%lu\","   \
    "\"file_size\":\"%lu\"}"

#define CANLOG_CONFIG_STRING                                                   \
    "{\"cluster_size\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}"

typedef struct
{
    uint8_t stage;
    uint32_t index;
    uint32_t callcount;
    uint32_t byteCount;
    char name[CANLOG_MAX_PATH_LENGTH];
} CustomHandlerState;

static CustomHandlerState reqState;
static FatFsDeviceType CanLogReadFileDevice;

/* accept only files inside /logs/ and beginning with CAN.LOG ---- */
int FsCustomLogs_OpenFile(struct fs_file *file, const char *name)
{
    uint32_t FileSize                     = 0U;
    char FileName[CANLOG_MAX_PATH_LENGTH] = FILEHANDLER_PARTITION_NO;

    reqState.index     = 0;
    reqState.stage     = 0;
    reqState.callcount = 0;
    reqState.byteCount = 0;

    strncat(FileName, name, sizeof(FileName) - strlen(FileName) - 1);
    strncpy((char *)reqState.name, FileName, sizeof(reqState.name) - 1);
    reqState.name[sizeof(reqState.name) - 1] = '\0';

    if (0 != FatFS_SD_OpenFileForRead(&CanLogReadFileDevice, FileName))
    {
        return 0;
    }

    if (FR_OK != FatFS_SD_GetFileSize(&CanLogReadFileDevice, &FileSize)
        || FileSize > INT_MAX)
    {
        (void)FatFS_SD_CloseFile(&CanLogReadFileDevice);
        return 0;
    }

    CanLogReadFileDevice.readTargetSize = FileSize;

    file->pextension     = &reqState;
    file->data           = NULL;
    file->len            = (int32_t)FileSize; // tell lwip the total file size
    file->index          = 0;
    file->is_custom_file = 1;

    return 1;
}

bool FsCustomLogs_RenderMeta(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    uint32_t HeadIndex;
    uint32_t TailIndex;
    uint32_t Capacity;
    uint32_t Progression;
    int DataSize;

    (void)FsCustom_GetCanLogHeadIndex(&HeadIndex);
    (void)FsCustom_GetCanLogTailIndex(&TailIndex);
    (void)FsCustom_GetCanLogCapacity(&Capacity);

    if (HeadIndex >= TailIndex)
    {
        Progression = HeadIndex - TailIndex + 1;
    }
    else if (HeadIndex < TailIndex)
    {
        Progression = Capacity + (HeadIndex - TailIndex + 1);
    }

    if (Progression < 2)
    {
        HeadIndex = TailIndex;
    }

    DataSize = snprintf(
        buffer,
        buffer_size,
        CANLOG_META_DATA_STRING,
        (unsigned long int)HeadIndex,
        (unsigned long int)TailIndex,
        (unsigned long int)Capacity,
        (unsigned long int)((HeadIndex + Capacity - 1) % Capacity),
        (unsigned long int)(appCanLogGetLogFileSize())
    );

    if (DataSize < 0 || (size_t)DataSize >= buffer_size)
    {
        return false; // Error: formatting failed or buffer too small
    }

    *payload_size = (size_t)DataSize;
    return true;
}

bool FsCustomLogs_RenderConfig(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    int DataSize;

    DataSize = snprintf(
        buffer,
        buffer_size,
        CANLOG_CONFIG_STRING,
        (unsigned long int)(appCanLogGetClusterSize()),
        (unsigned long int)(appCanLogGetLogFileSize()),
        (unsigned long int)(appCanLogGetLogFileCount())
    );

    if (DataSize < 0 || (size_t)DataSize >= buffer_size)
    {
        return false; // Error: formatting failed or buffer too small
    }

    *payload_size = (size_t)DataSize;
    return true;
}

int FsCustomLogs_Read(struct fs_file *file, char *buffer, int count)
{
    FRESULT fr;
    uint32_t len              = 0U;
    CustomHandlerState *state = (CustomHandlerState *)file->pextension;
    uint32_t ByteCount;

    if (state == NULL)
    {
        return FS_READ_EOF;
    }

    ByteCount = state->byteCount;

    if (ByteCount >= CanLogReadFileDevice.readTargetSize)
    {
        state->callcount = 0;
        state->byteCount = 0;
        return FS_READ_EOF;
    }

    if (count <= 0)
    {
        return FS_READ_EOF;
    }
    else if (CanLogReadFileDevice.readTargetSize - ByteCount > (uint32_t)count)
    {
        len = (uint32_t)count;
    }
    else
    {
        len = CanLogReadFileDevice.readTargetSize - ByteCount;
    }

    fr = FatFS_SD_ReadFile(&CanLogReadFileDevice, buffer, len);

    if (FR_OK != fr)
    {
        return FS_READ_EOF;
    }

    state->callcount++;
    state->byteCount += len;

    return (int)len; // triggers send
}

void FsCustomLogs_Close(struct fs_file *file)
{
    /* lwip's fs_open() forces is_custom_file to 1 for every fs_open_custom()
     * route, so fs_close() lands here for the rendered endpoints too. Only the
     * streamed CAN.LOG route owns CanLogReadFileDevice; closing it for anything
     * else aborts a download that is still in progress. */
    if (&reqState != file->pextension)
    {
        return;
    }

    FatFS_SD_CloseFile(&CanLogReadFileDevice);
    file->pextension = NULL; // optional cleanup
}

#endif
