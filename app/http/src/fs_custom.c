#include "lwip/apps/fs.h"
#include "lwip/def.h"

#include <string.h>

#include "FileHandler.h"

#include "test_can_trace.c"

struct fs_custom_data {
    FILE *f;
#if LWIP_HTTPD_EXAMPLE_CUSTOMFILES_DELAYED
    int delay_read;
    fs_wait_cb callback_fn;
    void *callback_arg;
#endif
};

#if LWIP_HTTPD_CUSTOM_FILES

#if LWIP_HTTPD_DYNAMIC_FILE_READ != 1
#warning "LWIP_HTTPD_DYNAMIC_FILE_READ is NOT enabled!"
#endif

typedef struct {
    uint8_t stage;
    uint32_t index;
    uint32_t callcount;
    const char name[256];
} CustomHandlerState;

static CustomHandlerState reqState;
static FatFsDeviceType CanLogReadFileDevice;

int fs_open_custom(struct fs_file *file, const char *name)
{
    const char CanLogFilename[] = "can.log";
    int FileSize = 0U;

    if (strcmp(name, "/can_trace") == 0) {
        reqState.index = 0;
        reqState.stage = 0;
        reqState.callcount = 0;
        strncpy((char *)reqState.name, name, sizeof(reqState.name));

        if ( 0 != FatFS_SD_OpenFileForRead(&(CanLogReadFileDevice.file), CanLogFilename) )
        {
            return 0;
        }

        (void) FatFS_SD_GetFileSize(&CanLogReadFileDevice.file, &FileSize);

        CanLogReadFileDevice.readTargetSize = FileSize;

        file->pextension = &reqState;
        file->data = NULL;
        file->len = FileSize;  // tell lwip the total file size
        file->index = 0;
        file->is_custom_file = 1;
    
        return 1;
    }
    return 0;  // Fallback to default file system
}

#define CAN_LOG_BUFFER_SIZE 8U

void fs_state_free(struct fs_file *file, void *state)
{
  LWIP_UNUSED_ARG(file);
  if (state != NULL) {
  }
}

#define CHUNK_SIZE  (512U)

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    uint32_t len = 0U;
    CustomHandlerState *state = (CustomHandlerState *)file->pextension;

    if (state == NULL) {
        return FS_READ_EOF;
    }

    if (strcmp(state->name, "/can_trace") == 0)
    {
        volatile uint32_t FileIndex = state->callcount * CHUNK_SIZE;
        
        if (FileIndex >= CanLogReadFileDevice.readTargetSize)
        {
            state->callcount = 0;
            return FS_READ_EOF;
        }

        if (CanLogReadFileDevice.readTargetSize > FileIndex + CHUNK_SIZE)
        {
            len = CHUNK_SIZE;
        }
        else
        {
            len = CanLogReadFileDevice.readTargetSize - FileIndex ;   
        }

        FatFS_SD_ReadFile(&CanLogReadFileDevice.file, buffer, len);

        state->callcount++;
    }
    
    return len; // triggers send
}

void fs_close_custom(struct fs_file *file)
{
    CustomHandlerState *state = (CustomHandlerState *)file->pextension;

    if (strcmp(state->name, "/can_trace") == 0)
    {
        FatFS_SD_CloseFile(&(CanLogReadFileDevice.file));
    }
        
    file->pextension = NULL; // optional cleanup
}

#endif
