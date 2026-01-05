#include "ff.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "fs_custom.h"
#include "FileHandler.h"

struct fs_custom_data {
    FILE *f;
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
    int delay_read;
    fs_wait_cb callback_fn;
    void *callback_arg;
#endif
};

static const char redirect_reply[] =
    "HTTP/1.1 303 See Other\r\n"
    "Location: /can.shtml\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "\r\n";

#if LWIP_HTTPD_CUSTOM_FILES

#if LWIP_HTTPD_DYNAMIC_FILE_READ != 1
#warning "LWIP_HTTPD_DYNAMIC_FILE_READ is NOT enabled!"
#endif

#define CANLOG_MAX_PATH_LENGTH    64U
#define CANLOG_MAX_META_DATA_SIZE 96U
#define CANLOG_MAX_STATUS_SIZE    80U
#define CANLOG_FILE_PATH          "/logs/CAN.LOG"
#define CANLOG_META_DATA_PATH     "/logs/meta"
#define CANLOG_STATUS_PATH        "/logger/status"
#define CANLOG_POST_REDIRECT_PATH "/postredir"

#define CANLOG_META_DATA_STRING \
    "{\"head\":%lu,\"tail\":%lu,\"capacity\":%lu,\"latest\":\"CAN.LOG%lu\"}"

#define CANLOG_STATUS_STRING \
    "{ \"active\":%s,\"frames_lost\":%s,\"bus_load_1\":%.2f,\"bus_load_2\":%.2f}"

typedef struct {
    uint8_t stage;
    uint32_t index;
    uint32_t callcount;
    uint32_t byteCount;
    char name[CANLOG_MAX_PATH_LENGTH];
} CustomHandlerState;

static CustomHandlerState reqState;
static FatFsDeviceType CanLogReadFileDevice;

/* WARNING: Not thread-safe. MetaData/StatusData shared across requests.
 * Assumes single-threaded or serialized HTTP request processing. */
static char MetaData[CANLOG_MAX_META_DATA_SIZE];
static char StatusData[CANLOG_MAX_STATUS_SIZE];

int fs_open_custom(struct fs_file *file, const char *name)
{
    uint32_t FileSize = 0U;
    
    /* accept only files inside /logs/ and beginning with CAN.LOG ---- */
    if (0 == strncmp(name, CANLOG_FILE_PATH, sizeof(CANLOG_FILE_PATH) - 1))
    {
        char FileName[CANLOG_MAX_PATH_LENGTH] = FILEHANDLER_PARTITION_NO;
        
        reqState.index = 0;
        reqState.stage = 0;
        reqState.callcount = 0;
        reqState.byteCount = 0;

        strncat(
            FileName, 
            name, 
            sizeof(FileName) - strlen(FileName) - 1
        );
        strncpy((char *)reqState.name, FileName, sizeof(reqState.name) - 1);
        reqState.name[sizeof(reqState.name) - 1] = '\0';

        if ( 0 != FatFS_SD_OpenFileForRead(&CanLogReadFileDevice, FileName) )
        {
            return 0;
        }

        (void) FatFS_SD_GetFileSize(&CanLogReadFileDevice, &FileSize);

        CanLogReadFileDevice.readTargetSize = FileSize;

        file->pextension = &reqState;
        file->data = NULL;
        file->len = FileSize;  // tell lwip the total file size
        file->index = 0;
        file->is_custom_file = 1;
    
        return 1;
    }
    else if (0 == strncmp(name, CANLOG_META_DATA_PATH, sizeof(CANLOG_META_DATA_PATH) - 1)) 
    {
        uint32_t HeadIndex;
        uint32_t TailIndex;
        uint32_t Capacity;
        uint32_t Progression;
        int DataSize;

        (void) FsCustom_GetCanLogHeadIndex(&HeadIndex);
        (void) FsCustom_GetCanLogTailIndex(&TailIndex);
        (void) FsCustom_GetCanLogCapacity(&Capacity);

        if (HeadIndex >= TailIndex)
        {
            Progression = HeadIndex - TailIndex + 1;
        }
        else if (HeadIndex < TailIndex)
        {
            Progression = Capacity + (HeadIndex - TailIndex + 1) ;
        }

        if (Progression < 2)
        {
            HeadIndex = TailIndex;
        }

        DataSize = snprintf(
            MetaData,
            sizeof MetaData,
            CANLOG_META_DATA_STRING,
            (unsigned long int)HeadIndex, 
            (unsigned long int)TailIndex, 
            (unsigned long int)Capacity, 
            (unsigned long int)((HeadIndex + Capacity - 1) % Capacity)
        );
    
        if (DataSize < 0 || (size_t)DataSize >= sizeof(MetaData)) {
            return 0;  // Error: formatting failed or buffer too small
        }

        file->data           = MetaData;
        file->len            = DataSize;
        file->index          = 0;
        file->is_custom_file = 0;       /* httpd sends static buffer     */
        return 1;
    }
    else if (0 == strncmp(name, CANLOG_STATUS_PATH, sizeof(CANLOG_STATUS_PATH) - 1)) {
        uint8_t IsTracerRunning = 1;
        float BusLoadCan1 = 0.0;
        float BusLoadCan2 = 0.0;
        _Bool AnyFrameLost = false;

        if (0 != FsCustom_IsTracerRunning(&IsTracerRunning))
        {
            IsTracerRunning =  1;
        }

        AnyFrameLost = FsCustom_IsAnyFrameLostFlag();
        FsCustom_GetBusloadCan1(&BusLoadCan1);
        FsCustom_GetBusloadCan2(&BusLoadCan2);

        int n = snprintf(StatusData,
            sizeof(StatusData),
            CANLOG_STATUS_STRING,
            IsTracerRunning ? "true" : "false",
            AnyFrameLost ? "true" : "false",
            BusLoadCan1,
            BusLoadCan2
        );
    
        file->data           = StatusData;
        file->len            = n;
        file->index          = 0;
        file->is_custom_file = 0;       /* httpd sends static buffer     */
        return 1;
    }
    else if (0 == strncmp(name, CANLOG_POST_REDIRECT_PATH, sizeof(CANLOG_POST_REDIRECT_PATH) - 1)) {
        file->data   = redirect_reply;
        file->len    = sizeof(redirect_reply) - 1;
        file->flags	= FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT;
        file->index  = 0;
        #if LWIP_HTTPD_DYNAMIC_HEADERS
            // file->http_header_included = 1;
        #endif
        #if LWIP_HTTPD_CUSTOM_FILES
            file->is_custom_file       = 1;
        #endif
        return 1;
    }

    return 0;  // Fallback to default file system
}

#define CAN_LOG_BUFFER_SIZE 8U

void fs_state_free(struct fs_file *file, void *state)
{
    LWIP_UNUSED_ARG(file);
    if (state != NULL) {
        if (state == &reqState) 
        { 
            (void) FatFS_SD_CloseFile(&CanLogReadFileDevice);
        }
    }
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    FRESULT fr;
    uint32_t len = 0U;
    CustomHandlerState *state = (CustomHandlerState *)file->pextension;
    uint32_t ByteCount;

    if (state == NULL) {
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
        len = count;
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
    
    return len; // triggers send
}

void fs_close_custom(struct fs_file *file)
{
    FatFS_SD_CloseFile(&CanLogReadFileDevice);
    file->pextension = NULL; // optional cleanup
}

#endif
