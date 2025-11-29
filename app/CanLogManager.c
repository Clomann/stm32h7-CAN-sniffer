#include <string.h>
#include <stdio.h>

#include "CanLogManager.h"

#include "CanLogManagerTypes.h"
#include "ErrorContext.h"
#include "ff.h"
#include "fs_custom.h"
#include "CanLogBuffer.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"
#include "SdBridgeTask.h"
#include "core_json.h"
#include "RuntimeChecks.h"

void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);

typedef struct
{
    uint32_t epoch;
    uint32_t fileIndex;
    uint32_t byteOffset;
    uint32_t crc;
} CanLogMetaDataType;

static volatile CanLogMetaDataType LogMetaData;
#if CANLOGAMANGER_PERSIST_METADATA
static uint8_t LogMetaDataOpenRes;
#endif

struct CanLogControlDataType
{
    struct
    {
        char *filename;
        uint32_t fnamemaxlen;
        uint32_t count;
        uint32_t timestamp;
        uint8_t openRes;
        uint32_t fileHeadIndex;
        uint32_t fileTailIndex;
        bool fileIndexWrapped;
        FatFsDeviceType writeFileDevice;
    } CanLog;
    uint8_t *mountRes;
    bool *runCanTracer;
    bool runCanTracerOld;
    bool *commitLog;
};

/**
 * @brief Hook called before flushing a CAN log block to storage.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_InstrumentationFlushStartHook(void) {}

/**
 * @brief Hook called after flushing a CAN log block to storage.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_InstrumentationFlushEndHook(void) {}

volatile static char CanLogFileName[255] = "/logs/CAN.LOG";
volatile static CanLogControlDataType CanLogCtrlData;

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix);
static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data);
static unsigned int appCanLogCheckNewFileOpen(CanLogControlDataType *data);
static void appCanLogFillEntry(
    CanLogClassicCanEntryType *entry,
    FDCAN_ClassicFrame *frame,
    uint64_t timestamp,
    uint8_t channel
);
static comm_status_t
appCanLogStoreToFrameBuffer(FDCAN_ClassicFrame *frame, uint8_t channel);
static comm_status_t
appCanLogStoreToSd(FatFsDeviceType *dev, char *data, uint32_t length);
static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev);

void __attribute__((weak)) CanLogFileManager_ErrorHandler()
{
    __asm volatile("nop");
}

uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index)
{
    *index = CanLogCtrlData.CanLog.fileHeadIndex;
    return 0U;
}

uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index)
{
    *index = CanLogCtrlData.CanLog.fileTailIndex;
    return 0U;
}

uint8_t FsCustom_GetCanLogCapacity(uint32_t *capacity)
{
    *capacity = MAX_LOG_INDEX + 1;
    return 0U;
}

uint8_t FsCustom_IsTracerRunning(uint8_t *running)
{
    uint8_t res;

    res      = 0;
    *running = *CanLogCtrlData.runCanTracer;

    return res;
}

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix)
{
    FatFS_FileIterator it;
    FILINFO *fno;
    int highest = -1;

    if (FatFS_SD_FileIterator_Open(&it, dirPath, prefix) != FR_OK)
    {
        return -1;
    }

    while (FatFS_SD_FileIterator_Next(&it, &fno) == FR_OK)
    {
        const char *suffix = fno->fname + strlen(prefix);
        char *endptr;
        long val = strtol(suffix, &endptr, 10);

        if (*endptr == '\0' && val >= 0 && val <= maxSuffix && val > highest)
        {
            highest = (int)val;
        }
    }

    FatFS_SD_FileIterator_Close(&it);
    return highest;
}

static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data)
{
    int lastUsed;
    volatile FRESULT res;

    (void)data;

    lastUsed = find_highest_suffix("/logs/", "CAN.LOG", MAX_LOG_INDEX);

#if 0U == PERSIST_CAN_LOG_FILE_HEAD_TAIL
    lastUsed = 0U;
#endif

    CanLogCtrlData.CanLog.fileHeadIndex = lastUsed;

    // Build candidate filename
    snprintf(
        CanLogCtrlData.CanLog.filename,
        CanLogCtrlData.CanLog.fnamemaxlen,
        FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
        (int)lastUsed
    );

    CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        CanLogCtrlData.CanLog.filename
    );

    res = f_lseek(&(CanLogCtrlData.CanLog.writeFileDevice.file), 0);

    return res;
}

#if CANLOGMANAGER_REOPEN_LOG_FILE

static CanLogResult appCanLogReopenFile(CanLogControlDataType *data)
{
    CanLogResult res;

    res = CAN_LOG_OK;

    if (FR_OK != FatFS_SD_OpenFileForWrite(
            &(CanLogCtrlData.CanLog.writeFileDevice), 
            data->CanLog.filename)
        )
    {
        res = CAN_LOG_NOT_OK;
        CanLogFileManager_ErrorHandler();
    }

    return res;
}

static CanLogResult appCanLogCloseFile(CanLogControlDataType *data)
{
    CanLogResult res;

    res = CAN_LOG_OK;

    if (FR_OK != FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice)))
    {
        res = CAN_LOG_NOT_OK;
        CanLogFileManager_ErrorHandler();
    }

    return res;
}

#endif

static unsigned int appCanLogCheckNewFileOpen(CanLogControlDataType *data)
{
    FRESULT FileSizeRes;
    uint32_t FileSize;

    (void)data;

    FileSizeRes = FatFS_SD_GetBufferedFileSize(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        &FileSize
    );

    if (FileSizeRes == FR_OK && (FileSize >= MAX_LOG_FILE_SIZE ) )
    {
        // File exists and is full, advance to next one
        if (0 == FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice)))
        {
            if (CanLogCtrlData.CanLog.fileHeadIndex >= MAX_LOG_INDEX)
            {
                CanLogCtrlData.CanLog.fileIndexWrapped = 1;
            }

            CanLogCtrlData.CanLog.fileHeadIndex =
                (CanLogCtrlData.CanLog.fileHeadIndex + 1) % (MAX_LOG_INDEX + 1);

            if (1 == CanLogCtrlData.CanLog.fileIndexWrapped)
            {
                CanLogCtrlData.CanLog.fileTailIndex =
                    (CanLogCtrlData.CanLog.fileHeadIndex + 1)
                    % (MAX_LOG_INDEX + 1);
            }

            snprintf(
                CanLogCtrlData.CanLog.filename,
                CanLogCtrlData.CanLog.fnamemaxlen,
                "/logs/CAN.LOG%d",
                (int)CanLogCtrlData.CanLog.fileHeadIndex
            );

#if !PREALLOCATE_LOG_FILES
            (void)f_unlink(CanLogCtrlData.CanLog.filename);
#endif

            CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForOverWrite(
                &(CanLogCtrlData.CanLog.writeFileDevice),
                CanLogCtrlData.CanLog.filename
            );

            if (0 != CanLogCtrlData.CanLog.openRes)
            {
                CanLogFileManager_ErrorHandler();
            }
            
            if (FR_OK != FatFS_SD_Flush(&(CanLogCtrlData.CanLog.writeFileDevice)))
            {
                CanLogFileManager_ErrorHandler();
            }
        }
        else
        {
            CanLogFileManager_ErrorHandler();
        }
    }

    return 0U;
}

CanLogControlDataType *CanLogHandler_Init(uint8_t *mount_res, bool *run, bool *commit)
{
    memset(&CanLogCtrlData, 0x0, sizeof(CanLogCtrlData));

    CanLogCtrlData.CanLog.filename    = CanLogFileName;
    CanLogCtrlData.CanLog.fnamemaxlen = sizeof(CanLogFileName);
    CanLogCtrlData.CanLog.openRes     = 1;

    CanLogCtrlData.mountRes     = mount_res;
    CanLogCtrlData.runCanTracer = run;
    CanLogCtrlData.commitLog = commit;

    return &CanLogCtrlData;
}

#if CANLOGMANAGER_CLEAR_ALL_LOGS

static FRESULT delete_all_files(const char* path) {
    DIR dir;
    FILINFO fno;
    FRESULT res;
    char full_path[256];
    
    // Open directory
    res = f_opendir(&dir, path);
    if (res != FR_OK) return res;
    
    // Read all entries
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break; // End of dir
        
        // Skip directories (optional - remove if you want to delete subdirs too)
        if (fno.fattrib & AM_DIR) continue;
        
        // Build full path
        sprintf(full_path, "%s/%s", path, fno.fname);
        
        // Delete the file
        res = f_unlink(full_path);
        if (res != FR_OK) {
            f_closedir(&dir);
            return res; // Return on error
        }
    }
    
    f_closedir(&dir);
    return FR_OK;
}

#endif

static bool m_verify_preallocation(const char* path) {
    FIL fil;
    FRESULT res;
    UINT bytes_read;
    BYTE dummy_byte = 0;
    bool can_seek;
    bool verified;
    
    res = f_open(&fil, path, FA_WRITE);
    if (res != FR_OK) return false;
    
    // Try seeking to near the expected pre-allocated size
    res = f_lseek(&fil, MAX_LOG_FILE_SIZE - 512U);
    can_seek = (res == FR_OK);

    res = f_read(&fil, &dummy_byte, 1, &bytes_read);
    verified = (res == FR_OK && bytes_read == 1);
    
    f_close(&fil);
    return can_seek && verified;
}

volatile static uint32_t UnseekableFiles = 0;

static FRESULT m_preallocate_log_files(void)
{
    FIL logfile;
    FRESULT res;
    UINT bytes_written;
    BYTE dummy_byte = 0;
    char full_path[256];

    UnseekableFiles = 0;

    for (uint32_t i = 0; i < MAX_LOG_FILE_COUNT; i++)
    {
        snprintf(full_path, sizeof(full_path), FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d", (int)i);
        
        // Check if file already exists and is properly sized
        if (1U == m_verify_preallocation(full_path)) {
            // File exists and is properly sized - skip
            continue;
        }

        UnseekableFiles++;

        // File doesn't exist or is too small - create/resize it
        res = f_open(&logfile, full_path, FA_WRITE | FA_CREATE_NEW);
        if (res == FR_EXIST) {
            // File exists but is too small - open for expansion
            res = f_open(&logfile, full_path, FA_WRITE);
        }
        
        if (res != FR_OK) {
            continue;
        }
        
        if (res == FR_OK) {
            res = f_expand(&logfile, MAX_LOG_FILE_SIZE, 0);

            if (res == FR_OK) 
            {
                
                res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 512U);
                if (res != FR_OK) 
                {
                    res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 1024U);
                }
                if (res != FR_OK) 
                {
                    res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 3U*512U);
                }
                if (res != FR_OK) 
                {
                    res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 4U*512U);
                }
                if (res != FR_OK) 
                {
                    res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 5U*512U);
                }
            }

            if (res == FR_OK) 
            {
                res = f_write(&logfile, &dummy_byte, 1, &bytes_written);
            }
            
            if (res == FR_OK && bytes_written == 1) 
            {
                res = f_lseek(&logfile, 0);
            }
        }
        
        f_close(&logfile);
    }

    CanLogCtrlData.CanLog.fileHeadIndex = 0;

    // Build candidate filename
    snprintf(
        CanLogCtrlData.CanLog.filename,
        CanLogCtrlData.CanLog.fnamemaxlen,
        FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d",
        0
    );

    CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        CanLogCtrlData.CanLog.filename
    );

    return FR_OK;
}

#if CANLOGAMANGER_PERSIST_METADATA

static int CanLogManager_ParseMetaData(char *buffer, uint32_t len, CanLogMetaDataType *meta)
{
    // Variables used in this example.
    JSONStatus_t result;
    char TmpBuf[64];
    char * value;
    size_t valueLength;
    size_t bufferLength = len;
    uint32_t Epoch;
    const char queryKey1[] = "epoch";
    const size_t queryKeyLength1 = sizeof( queryKey1 ) - 1;
    uint32_t FileIndex;
    const char queryKey2[] = "index";
    const size_t queryKeyLength2 = sizeof( queryKey2 ) - 1;
    uint32_t ByteOffset;
    const char queryKey3[] = "offset";
    const size_t queryKeyLength3 = sizeof( queryKey3 ) - 1;
    uint32_t Crc;
    const char queryKey4[] = "crc";
    const size_t queryKeyLength4 = sizeof( queryKey4 ) - 1;

    result = JSON_Validate( buffer, bufferLength );    

    if( result == JSONSuccess )
    {
        result = FileHandler_GetValue( buffer, bufferLength, queryKey1, queryKeyLength1, &value, &valueLength);
        if( JSONSuccess ==  result )
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if ( 0U == FileHandler_ConvertToInteger(TmpBuf, &Epoch, 10U) )
                meta->epoch = Epoch;
        }
        
        result = FileHandler_GetValue( buffer, bufferLength, queryKey2, queryKeyLength2, &value, &valueLength );
        if( JSONSuccess == result ) 
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &FileIndex, 10U))
                meta->fileIndex = FileIndex;
        }

        result = FileHandler_GetValue( buffer, bufferLength, queryKey3, queryKeyLength3, &value, &valueLength);
        if( JSONSuccess ==  result )
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if ( 0U == FileHandler_ConvertToInteger(TmpBuf, &ByteOffset, 10U) )
                meta->byteOffset = ByteOffset;
        }
        
        result = FileHandler_GetValue( buffer, bufferLength, queryKey4, queryKeyLength4, &value, &valueLength );
        if( JSONSuccess == result ) 
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &Crc, 10U))
                meta->crc = Crc;
        }
    }

    if (JSONSuccess == result)
    {
        return CANLOG_E_OK;
    }
    else
    {
        return CANLOG_E_NOT_OK;
    }
}

static FRESULT m_MetaDataLoad(CanLogMetaDataType *data)
{
    FILINFO info;
    FRESULT res;
    FILINFO fno;
    FatFsDeviceType Dev;
    char Content[128];
    uint32_t BufferSize;
    uint32_t FileSize;
    uint32_t ReadSize;
    ErrorContextType ErrorContext;

    res = f_stat(CAN_LOG_META_FILENAME, &fno);

    switch (res) 
    {
    case FR_OK:
        res = FatFS_SD_OpenFileForRead(&Dev, CAN_LOG_META_FILENAME);
        break;
    case FR_NO_FILE:
    case FR_NO_PATH:
        break;
    default:
    }

    if (FR_OK == res)
    {
        BufferSize = sizeof(Content);
        
        res = FatFS_SD_GetFileSize(&Dev, &FileSize);
        if (res != FR_OK)
        {
            res = CANLOG_E_FILE_READ;
        }

        
        if (CANLOG_E_OK == res)
        {
            // Read data
            ReadSize = (FileSize < BufferSize) ? FileSize : BufferSize;
            res = FatFS_SD_ReadFile(&Dev, Content, ReadSize);
                    
            if (CANLOG_E_OK != res)
            {
                res = CANLOG_E_FILE_READ;
            }

        }   

        if (FR_OK == res)
        {
            res = CanLogManager_ParseMetaData(Content, ReadSize, data);
        }

        res = FatFS_SD_CloseFile(&Dev);

        if (FR_OK != res)
        {
            ErrorContext.code = res;
            ErrorContext.line = __LINE__;
            snprintf(
                ErrorContext.function, 
                sizeof(ErrorContext.function),
                "%s",
                "SD_Spi_writeMultiBlock"
            );

            CanLogFileManager_ErrorHandler();
        }
    }
    else
    {
        res = CANLOG_E_FILE_OPEN;
    }

    return res;
}

static uint8_t m_MetaDataToJSonString(CanLogMetaDataType *data, char *json, uint32_t maxLength, uint32_t *len)
{
    // Create the JSON string
    snprintf(json, maxLength,
        "{\n"
        "    \"epoch\":%d,\n"
        "    \"index\":%d,\n"
        "    \"offset\":%d,\n"
        "    \"crc\":%d\n"
        "}\n",
        data->epoch,
        data->fileIndex,
        data->byteOffset,
        data->crc
    );

    *len = strnlen(json, maxLength);
    return 0U;
}

static FRESULT m_MetaDataStore(CanLogMetaDataType *data)
{
    FILINFO info;
    FRESULT res;
    FatFsDeviceType Dev;
    char Content[128];
    uint32_t BufferSize;
    uint32_t BytesWritten;
    uint32_t StringSize;
    uint32_t WriteSize;

    res = FatFS_SD_OpenFileForOverWrite(&Dev, CAN_LOG_META_FILENAME);

    if (FR_OK == res)
    {
        BufferSize = sizeof(Content);

        (void) m_MetaDataToJSonString(data, Content, BufferSize, &StringSize);
        
        if (CANLOG_E_OK == res)
        {
            // Read data
            WriteSize = (StringSize < BufferSize) ? StringSize : BufferSize;
            res = FatFS_SD_WriteFile(&Dev, Content, WriteSize);
                    
            if (CANLOG_E_OK != res)
            {
                res = CANLOG_E_FILE_WRITE;
                CanLogFileManager_ErrorHandler();
            }
        }   
        else
        {
            CanLogFileManager_ErrorHandler();
        }

        FatFS_SD_CloseFile(&Dev);
    }
    else
    {
        res = CANLOG_E_FILE_OPEN;
    }

    return res;
}
#endif

FRESULT appCanLogHandlerInit(CanLogControlDataType *data)
{
    FILINFO info;
    FRESULT res;

#if CANLOGAMANGER_PERSIST_METADATA
    memset(&LogMetaData, 0, sizeof(LogMetaData));

    LogMetaDataOpenRes = m_MetaDataLoad(&LogMetaData);

    if (0 != LogMetaDataOpenRes)
    {
        CanLogFileManager_ErrorHandler();
    }

    LogMetaData.epoch++;

    CanLogBuffer_SetEpochCount(LogMetaData.epoch);

    m_MetaDataStore(&LogMetaData);
#endif

    *CanLogCtrlData.runCanTracer           = false;
    CanLogCtrlData.runCanTracerOld         = false;
    CanLogCtrlData.CanLog.fileHeadIndex    = 0;
    CanLogCtrlData.CanLog.fileTailIndex    = 0;
    CanLogCtrlData.CanLog.fileIndexWrapped = 0;

    CanLogBuffer_Init();

    fdcan_msg_port_init();

#if CANLOGMANAGER_CLEAR_ALL_LOGS
    res = f_stat(FILEHANDLER_PARTITION_NO "/logs", &info);

    if ((res == FR_OK) && (info.fattrib & AM_DIR))
    {
        if (FR_OK == delete_all_files(FILEHANDLER_PARTITION_NO "/logs"))
        {
            f_rmdir(FILEHANDLER_PARTITION_NO "/logs");
        }
    }
#endif

    res = f_stat(FILEHANDLER_PARTITION_NO "/logs", &info);

    if ((res == FR_OK) && (info.fattrib & AM_DIR))
    {
    }
    else if (res == FR_NO_FILE)
    {
        // Directory does not exist
        res = f_mkdir(FILEHANDLER_PARTITION_NO "/logs");
        if (res != FR_OK)
        {
            CanLogFileManager_ErrorHandler();
        }
    }
    else if ((res == FR_OK) && (!(info.fattrib & AM_DIR)))
    {
        CanLogFileManager_ErrorHandler();
    }

#if PREALLOCATE_LOG_FILES
    if (RES_OK == *CanLogCtrlData.mountRes)
    {
        m_preallocate_log_files();
    }
    else
    {
    }
#endif

    if (RES_OK == *CanLogCtrlData.mountRes)
    {
        appCanLogOpenMostRecentFile(data);
    }
    else
    {
        CanLogCtrlData.CanLog.openRes = 1U;
    }

    return res;
}

static void appCanLogFillEntry(
    CanLogClassicCanEntryType *entry,
    FDCAN_ClassicFrame *frame,
    uint64_t timestamp,
    uint8_t channel
)
{
    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CANLOG_CLASSIC_TYPE;
    entry->header.total_len  = sizeof(CanLogClassicCanEntryType);
    entry->timestamp         = timestamp;
    entry->channel           = channel;
    entry->dlc               = frame->dlc;
    entry->can_id = frame->id;
    memcpy(entry->data, frame->data, sizeof(entry->data));
}

static comm_status_t
appCanLogStoreToFrameBuffer(FDCAN_ClassicFrame *frame, uint8_t channel)
{
    comm_status_t res = COMM_SUCCESS;
    CanLogClassicCanEntryType NewEntry;

    appCanLogFillEntry(
        &NewEntry,
        frame,
        frame->timestamp,
        channel
    );

    CanLogBuffer_AddClassicCanEntry(&NewEntry);

    return res;
}

static comm_status_t
appCanLogStoreToSd(FatFsDeviceType *dev, char *data, uint32_t length)
{
    comm_status_t res = COMM_SUCCESS;

    if (FR_OK == FatFS_SD_WriteFile(dev, (const char *)data, length))
    {
        if (FR_OK != FatFS_SD_Flush(dev))
        {
            res = COMM_ERROR;
        }
    }
    else
    {
        res = COMM_ERROR;
    }

    (void) FatFS_SD_GetBufferedFileSize(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        &LogMetaData.byteOffset
    );

    LogMetaData.fileIndex = CanLogCtrlData.CanLog.fileHeadIndex;

    return res;
}

static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev)
{
    uint32_t DataLength;
    comm_status_t res               = COMM_SUCCESS;
    volatile static uint8_t Data[BLOCK_SIZE] = {0};

    if (CANLOG_E_OK == CanLogBuffer_ReadNextBlock(Data, &DataLength))
    {
        CanLogManager_InstrumentationFlushStartHook();
        res = appCanLogStoreToSd(dev, (char *)Data, DataLength);
        CanLogManager_InstrumentationFlushEndHook();

        if (COMM_SUCCESS == res)
        {
            CanLogBuffer_BlockCount++;
        }
    }
    else
    {
        res = COMM_ERROR;
    }
    
    return res;
}

void SdBridgeTask_ActionHook()
{
    appCanLogStoreBlock(&(CanLogCtrlData.CanLog.writeFileDevice));
}

void appCanLogHandlerPoll(CanLogControlDataType *data)
{
    comm_status_t res = COMM_SUCCESS;
    bool IsOffState;
    uint8_t BlockIsReady;
    uint32_t SlotsToWrite = 0;
    FDCAN_ClassicFrame NewFrame;

    if (CanLogCtrlData.runCanTracerOld == *CanLogCtrlData.runCanTracer)
    {
    }
    else if (true == *CanLogCtrlData.runCanTracer)
    {
#if CANLOGMANAGER_REOPEN_LOG_FILE
        if (CAN_LOG_OK != appCanLogReopenFile(data))
        {
            CanLogFileManager_ErrorHandler();
        }
#endif

        CanAbs_IsStateOff_Can1(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can1())
        {
            res = COMM_ERROR;
            CanLogFileManager_ErrorHandler();
        }

        CanAbs_IsStateOff_Can2(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can2())
        {
            res = COMM_ERROR;
            CanLogFileManager_ErrorHandler();
        }

        if (COMM_SUCCESS != res)
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        CanLogCtrlData.runCanTracerOld = *CanLogCtrlData.runCanTracer;
    }
    else
    {
        if (COMM_SUCCESS != CanAbs_Stop_Can1())
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        if (COMM_SUCCESS != CanAbs_Stop_Can2())
        {
            *CanLogCtrlData.runCanTracer = false;
        }

        CanLogCtrlData.runCanTracerOld = *CanLogCtrlData.runCanTracer;

        *(CanLogCtrlData.commitLog) = true;
    }

    appCanLogCheckNewFileOpen(data);

    while (0 < fdcan_msg_port_read(&NewFrame, 0))
    {
        CanLogManager_FrameCount++;

        appCanLogStoreToFrameBuffer(&NewFrame, NewFrame.channel);

        CanLogBuffer_IsBlockReady(&BlockIsReady);
        
        if (0 != CanLogCtrlData.CanLog.openRes)
        {
            /* quit */
            CanLogFileManager_ErrorHandler();
        }
        else if (BlockIsReady)
        {
            SdBridgeTask_Notify();
        }
        else
        {
        }
    }

    if (*(CanLogCtrlData.commitLog))
    {    
        CanLogBuffer_UsedSlots((uint8_t *)&SlotsToWrite);

        if (SlotsToWrite > 0)
        {
            SdBridgeTask_Notify();
        }

#if CANLOGMANAGER_REOPEN_LOG_FILE
        if (CAN_LOG_OK != appCanLogCloseFile(data))
        {
            CanLogFileManager_ErrorHandler();
        }
#endif

        *(CanLogCtrlData.commitLog) = false;
    }
}

void appCanLogHandlerDeInit(CanLogControlDataType *data)
{
    (void)data;

    if (0 == CanLogCtrlData.CanLog.openRes)
    {
        FatFS_SD_CloseFile(&(CanLogCtrlData.CanLog.writeFileDevice));
    }
}
