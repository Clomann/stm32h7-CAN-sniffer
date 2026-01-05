#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "CanLogManager.h"

#include "CanLogManagerTypes.h"
#include "ErrorContext.h"
#include "SettingsHandler.h"
#include "ff.h"
#include "fs_custom.h"
#include "CanLogBuffer.h"
#include "CanAbs.h"
#include "CanCtrl.h"
#include "fdcan_msg_port.h"
#include "SdBridgeTask.h"
#include "core_json.h"
#include "RuntimeChecks.h"

#include "instrumentation.h"
#if INSTR_ENABLED
#include "CommTypes.h"
#include "FileHandler.h"
#endif

#define CLM_ABS_TIME_TO_TIMSTAMP(x)   (uint32_t)(x)
#define CLM_ABS_TIME_TO_ABS_HIGH(x)   ((uint32_t)((x) >> 32U))
#define CLM_SYNC_EMIT_INTERVAL_US     (30ULL * 60ULL * 1000000ULL)

extern uint64_t FDCAN_GetTimestampHook(void);

void CanLogManager_InstrumentationFlushStartHook(void);
void CanLogManager_InstrumentationFlushEndHook(void);
void CanLogManager_DrainPortStartHook(void);
void CanLogManager_DrainPortEndHook(void);

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

typedef struct 
{
    uint64_t receivedFrames;
    uint32_t prevTimestamp;
    uint32_t baudrate;
    float busLoad;
    uint32_t accumulatedBits;
    uint32_t accumulatedTime;
} CanStatusDataType;

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
    CanStatusDataType Can1;
    CanStatusDataType Can2;
    uint8_t *mountRes;
    bool *runCanTracer;
    bool *commitLog;
    bool runCanTracerOld;
    bool framesLost;
    bool emitSyncEntry;
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

/**
 * @brief  Hook called before reading all available frames in the fdcan port buffer.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_DrainPortStartHook(void) {}

/**
 * @brief Hook called after reading all available frames in the fdcan port buffer.
 * @note Implemented by the application layer (e.g., GPIO toggle, timestamping, trace).
 */
__attribute__((weak)) void CanLogManager_DrainPortEndHook(void) {}

volatile static char CanLogFileName[255] = "/logs/CAN.LOG";
volatile static CanLogControlDataType CanLogCtrlData;

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix);
static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data);
static unsigned int appCanLogCheckNewFileOpen(CanLogControlDataType *data);
static void appCanLogFillEntry(
    CanLogEntryType *entry,
    FDCAN_ClassicFrameType *frame,
    uint64_t timestamp,
    uint8_t channel
);
static comm_status_t
appCanLogStoreToFrameBuffer(void *entry);
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

_Bool FsCustom_IsAnyFrameLostFlag(void)
{
    return CanLogCtrlData.framesLost;
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

uint8_t FsCustom_GetBusloadCan1(float *busload)
{
    *busload = CanLogCtrlData.Can1.busLoad;
    return 0U;
}

uint8_t FsCustom_GetBusloadCan2(float *busload)
{
    *busload = CanLogCtrlData.Can2.busLoad;
    return 0U;
}

static bool appCanLogIsValidBaudrate(uint32_t value)
{
    return (value == 250000U) || (value == 500000U) || (value == 1000000U);
}

CanLogResult appCanLogSetParam(ClmParameterIdType id, uint32_t value)
{
    CanLogResult res = CAN_LOG_OK;

    switch (id)
    {
        case CLM_PARAMETER_ID_CAN1_BAUDRATE:
            if (!appCanLogIsValidBaudrate(value))
            {
                res = CAN_LOG_ERR_INVALID_PARAM;
                break;
            }
            CanLogCtrlData.Can1.baudrate = value;
            break;
        case CLM_PARAMETER_ID_CAN2_BAUDRATE:
            if (!appCanLogIsValidBaudrate(value))
            {
                res = CAN_LOG_ERR_INVALID_PARAM;
                break;
            }
            CanLogCtrlData.Can2.baudrate = value;
            break;
        default:
            res = CAN_LOG_ERR_INVALID_PARAM;
            break;
    }

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

    CanLogCtrlData.Can1.busLoad = 0.0;
    CanLogCtrlData.Can1.prevTimestamp = 0;
    CanLogCtrlData.Can1.accumulatedBits = 0;
    CanLogCtrlData.Can1.accumulatedTime = 0;

    CanLogCtrlData.Can2.busLoad = 0.0;
    CanLogCtrlData.Can2.prevTimestamp = 0;
    CanLogCtrlData.Can2.accumulatedBits = 0;
    CanLogCtrlData.Can2.accumulatedTime = 0;

    RuntimeChecks_Init();

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
    UINT bytes_to_read;
    BYTE dummy_bytes[1U] ={0};
    bool can_seek;
    bool verified;
    
    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) return false;
    
    // Try seeking to near the expected pre-allocated size
    res = f_lseek(&fil, MAX_LOG_FILE_SIZE - sizeof(dummy_bytes));
    can_seek = (res == FR_OK);

    bytes_to_read = sizeof(dummy_bytes);
    res = f_read(&fil, dummy_bytes, bytes_to_read, &bytes_read);
    verified = (res == FR_OK && bytes_read > 0U);
    
    f_close(&fil);
    return can_seek;
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

    snprintf(full_path, sizeof(full_path), FILEHANDLER_PARTITION_NO "/logs/CAN.LOG%d", (int)MAX_LOG_INDEX);

    if (1U != m_verify_preallocation(full_path))
    {
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
                    res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 1U);
                    
                    if (res != FR_OK) 
                    {
                        res = f_lseek(&logfile, MAX_LOG_FILE_SIZE - 512U);
                    }
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
                    res = f_write(&logfile, &dummy_byte, 1U, &bytes_written);
                }
                
                if (res == FR_OK && bytes_written == 1) 
                {
                    res = f_lseek(&logfile, 0);
                }
            }
            
            f_close(&logfile);
        }
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
    default:
        break;
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
        "    \"epoch\":%" PRIu32 ",\n"
        "    \"index\":%" PRIu32 ",\n"
        "    \"offset\":%" PRIu32 ",\n"
        "    \"crc\":%" PRIu32 "\n"
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
    FRESULT res;
    FatFsDeviceType Dev;
    char Content[128];
    uint32_t BufferSize;
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
    CanLogEntryType *entry,
    FDCAN_ClassicFrameType *frame,
    uint64_t timestamp,
    uint8_t channel
)
{
    ClbDlcFlagsType Flags = 0;
    uint8_t Dlc;
    uint8_t DataLen;

    Dlc = FDCAN_GET_DLC(frame);
    Flags = FDCAN_GET_BRS(frame) << CAN_FLAG_BRS_Pos
        | FDCAN_GET_ESI(frame) << CAN_FLAG_ESI_Pos
        | FDCAN_GET_FDF(frame) << CAN_FLAG_RTR_FDF_Pos
        | FDCAN_GET_IDE(frame) << CAN_FLAG_IDE_Pos;
    DataLen = FDCAN_GET_DATA_LEN(frame);

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->header.total_len  = sizeof(CanLogEntryType) + DataLen;
    entry->timestamp         = timestamp;
    entry->channel           = channel;
    entry->dlc_flags         = MAKE_DLC_FLAGS(Dlc, Flags);
    entry->data_len          = DataLen;
    entry->can_id            = frame->id;

    memcpy(entry->data, frame->data, DataLen);
}

#if INSTR_ENABLED
#if INSTR_PERSIST_ACTIVE
static InstrErrorType appPersistInstrumentationData(void)
{
    FRESULT res;
    FatFsDeviceType File;
    uint32_t BytesToWrite = 0;
    uint8_t * Data = NULL;
    const char FileName[] = "InstrumentationData.bin";

    res = FatFS_SD_OpenFileForOverWrite(&File, FileName);
    
    if (FR_OK == res)
    {
        Instrumentation_SerializeHook(Data, &BytesToWrite);
        FatFS_SD_WriteFile(&File, (const char *)Data, BytesToWrite);
    }

    FatFS_SD_CloseFile(&File);

    return res;
}
#endif
#endif

static comm_status_t
appCanLogStoreToFrameBuffer(void *entry)
{
    comm_status_t res = COMM_SUCCESS;
    CanLogEntryHeaderType *pHeader;

    pHeader = (CanLogEntryHeaderType *)entry;
    
    if (0 != CanLogBuffer_AddEntry(entry, pHeader->total_len))
    {
        CanLogManager_FrameDropCount1++;
    }

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
    uint32_t FrameCount;
    uint8_t *DataPtr;
    comm_status_t res = COMM_SUCCESS;

    if (CANLOG_E_OK == CanLogBuffer_ReadNextBlock(&DataPtr, &DataLength, &FrameCount))
    {
        CanLogManager_InstrumentationFlushStartHook();
        res = appCanLogStoreToSd(dev, (char *)DataPtr, DataLength);
        CanLogManager_InstrumentationFlushEndHook();

        CanLogBuffer_Consume(DataLength, FrameCount);

        if (COMM_SUCCESS == res)
        {
            CanLogBuffer_BlockCount++;
        }
        else
        {
            CanLogManager_FrameDropCount2 += FrameCount;
        }
    }
    else
    {
        res = COMM_ERROR;
    }
    
    return res;
}

static comm_status_t CanLogManager_EmitSyncEntry(
    CanLogSyncType *sync,
    uint64_t absTime,
    uint32_t frameTimestamp
)
{
    /* Keep the 32-bit timestamp aligned with the originating frame to avoid
     * offsets when the absolute 64-bit timer wraps. */
    sync->timestamp = CLM_ABS_TIME_TO_TIMSTAMP(frameTimestamp);
    sync->abs_time_high  = CLM_ABS_TIME_TO_ABS_HIGH(absTime);

    sync->header.header_len = sizeof(sync->header);
    sync->header.type       = CLB_ENTRY_TYPE_SYNC;
    sync->header.total_len  = sizeof(CanLogSyncType);

    return appCanLogStoreToFrameBuffer((void *)sync);
}

void SdBridgeTask_ActionHook()
{
    appCanLogStoreBlock(&(CanLogCtrlData.CanLog.writeFileDevice));
}

CanLogResult m_ComputeFrameBits(CanLogEntryType *frame, uint32_t *bits)
{
    CanLogResult res = CAN_LOG_OK;
    uint8_t Dlc;
    uint32_t FrameBits = 0;
    bool IsCanfd;
    bool IsExtended;

    Dlc = GET_DLC(frame->dlc_flags) >> CAN_DLC_MASK_Pos;

    switch (Dlc)
    {
        case  9: Dlc = 12; break;
        case 10: Dlc = 16; break;
        case 11: Dlc = 20; break;
        case 12: Dlc = 24; break;
        case 13: Dlc = 32; break;
        case 14: Dlc = 48; break;
        case 15: Dlc = 64; break;
        default: 
            break;
    }

    IsCanfd = (frame->dlc_flags & CAN_FLAG_RTR_FDF);
    IsExtended = (frame->dlc_flags & CAN_FLAG_IDE);

    if (IsCanfd) {
        // CAN FD frame structure
        if (IsExtended) {
            // Extended ID: 1 + 32 + 2 + 1 + 1 + 4 + data + CRC + 2 + 7 + 3 = 53 + data + CRC
            FrameBits = 53 + (Dlc * 8);
            // CRC: 17 bits for ≤16 bytes, 21 bits for >16 bytes
            FrameBits += (Dlc <= 16) ? 17 : 21;
        } else {
            // Standard ID: 1 + 12 + 2 + 1 + 1 + 4 + data + CRC + 2 + 7 + 3 = 33 + data + CRC
            FrameBits = 33 + (Dlc * 8);
            FrameBits += (Dlc <= 16) ? 17 : 21;
        }
    } else {
        // Classic CAN frame structure
        if (IsExtended) {
            // Extended: 1 + 32 + 6 + data + 15 + 1 + 2 + 7 + 3 = 67 + data
            FrameBits = 67 + (Dlc * 8);
        } else {
            // Standard: 1 + 11 + 6 + data + 15 + 1 + 2 + 7 + 3 = 47 + data
            FrameBits = 47 + (Dlc * 8);
        }
    }
    
    *bits = FrameBits;

    return res;
}

CanLogResult m_ComputeBusLoad(
    CanStatusDataType *can,
    CanLogEntryType *frame
)
{
    CanLogResult res = CAN_LOG_OK;
    float Period = 0.0f; // in seconds
    uint32_t FrameBits = 0;
    uint32_t TimeDiff = 0;
    uint32_t ts;
    uint32_t prev;
    uint32_t diff;

    if (NULL == can || NULL == frame)
    {
        res = CAN_LOG_ERR_INVALID_POINTER;
        return res;
    }
    else if (can->baudrate == 0)
    {
        res = CAN_LOG_ERR_INVALID_PARAM;
        return res;
    }

    if (can->prevTimestamp == 0) {
        can->prevTimestamp = frame->timestamp;
        return res;
    }

    ts   = frame->timestamp;
    prev = can->prevTimestamp;
    diff = 0U;

    if (ts >= prev)
    {
        diff = ts - prev;
        can->prevTimestamp = ts;
    }
    else // prev > ts 
    {
        if ((prev > UINT32_MAX/2U) && (ts < UINT32_MAX/2U))
        {
            diff = (UINT32_MAX - prev) + ts + 1U;
            can->prevTimestamp = ts;
        }
    }

    TimeDiff = diff;

    m_ComputeFrameBits(frame, &FrameBits);

    can->accumulatedBits += FrameBits;
    can->accumulatedTime += TimeDiff;

    // Update every 1 second
    if (can->accumulatedTime >= 1000000U) {
        Period = can->accumulatedTime / 1000000.0f;
        can->busLoad = ((can->accumulatedBits * 1.1f) / (can->baudrate * Period)) * 100.0f;

        if (can->busLoad > 0.0f && can->busLoad < 0.01f)
        {
            can->busLoad = 0.01f;
        }

        can->accumulatedBits = 0;
        can->accumulatedTime = 0;
    }

    return res;
}

CanLogResult m_ComputeBusLoad1(
    CanStatusDataType *can, 
    CanLogEntryType *frame
)
{
    return m_ComputeBusLoad(can, frame);
}

CanLogResult m_ComputeBusLoad2(
    CanStatusDataType *can, 
    CanLogEntryType *frame
)
{
    return m_ComputeBusLoad(can, frame);
}

void m_DecayBusLoad(CanStatusDataType * can, uint32_t currentTime)
{
    int64_t TimeDiff;

    TimeDiff = currentTime - can->prevTimestamp;

    if (TimeDiff < 0)
    {
        TimeDiff = (uint32_t)((int64_t)UINT32_MAX + TimeDiff) + 1U;
    }

    if (TimeDiff > 1000000)
    {
        can->busLoad *= 0.75f;
        if (can->busLoad < 0.01f) {
            can->busLoad = 0.0f;
        }
        can->accumulatedBits = 0;
        can->accumulatedTime = 0; 
    }
}

void appCanLogHandlerPoll(CanLogControlDataType *data)
{
    comm_status_t res = COMM_SUCCESS;
    bool IsOffState;
    uint8_t BlockIsReady;
    uint32_t SlotsToWrite = 0;
    FDCAN_ClassicFrameType *pNewFrame;
    volatile uint64_t AbsTime = 0;
    RuntimeChecksContextType ErrorContext;
    CanLogSyncType SyncEntry;
    CanLogEntryStackBufferType EntryBuffer;
    CanLogEntryType *pFrameEntry = (CanLogEntryType *)(&EntryBuffer);
    volatile uint32_t LocalFrameCount;
    static uint64_t NextPeriodicSyncAbsTime = 0;
    static uint32_t LastFrameTimestamp = 0;
    static bool LastFrameTimestampValid = false;

    RuntimeChecks_CheckFrameCounts(&ErrorContext);

    if (RUNTIMECHECKS_E_FRAMES_DROPPED == ErrorContext.err)
    {
        CanLogCtrlData.framesLost = true;
    }

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

        CanLogCtrlData.emitSyncEntry = true;
        AbsTime = FDCAN_GetTimestampHook();
        NextPeriodicSyncAbsTime = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
        LastFrameTimestampValid = false;

        CanLogCtrlData.framesLost = false;
        RuntimeChecks_Init();
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

        CanAbs_Drain();

        fdcan_msg_port_flush();
    }

    appCanLogCheckNewFileOpen(data);

    (void) LocalFrameCount;
    LocalFrameCount = 0;

    AbsTime = FDCAN_GetTimestampHook();

    m_DecayBusLoad(&CanLogCtrlData.Can1, CLM_ABS_TIME_TO_TIMSTAMP(AbsTime));
    m_DecayBusLoad(&CanLogCtrlData.Can2, CLM_ABS_TIME_TO_TIMSTAMP(AbsTime));

    if ((0ULL != NextPeriodicSyncAbsTime) && (AbsTime >= NextPeriodicSyncAbsTime))
    {
        CanLogCtrlData.emitSyncEntry = true;
        NextPeriodicSyncAbsTime = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
    }

    while (0 < fdcan_msg_port_read(&pNewFrame, 2))
    {
        CanLogManager_DrainPortStartHook();
        CanLogManager_FrameCount++;
        LocalFrameCount++;

        AbsTime = FDCAN_GetTimestampHook();

        if ((0ULL != NextPeriodicSyncAbsTime) && (AbsTime >= NextPeriodicSyncAbsTime))
        {
            CanLogCtrlData.emitSyncEntry = true;
            NextPeriodicSyncAbsTime = AbsTime + CLM_SYNC_EMIT_INTERVAL_US;
        }

        if (true == CanLogCtrlData.emitSyncEntry)
        {
            CanLogCtrlData.emitSyncEntry = false;
            if (COMM_SUCCESS != CanLogManager_EmitSyncEntry(
                        &SyncEntry,
                        AbsTime,
                        pNewFrame->timestamp))
            {
                CanLogFileManager_ErrorHandler();
            }
        }

        appCanLogFillEntry(
            pFrameEntry,
            pNewFrame,
            pNewFrame->timestamp,
            pNewFrame->channel
        );

        if (pFrameEntry->channel == 1)
        {
            CanLogCtrlData.Can1.receivedFrames++;
            m_ComputeBusLoad1(
                &CanLogCtrlData.Can1,
                pFrameEntry
            );
        }
        else if (pFrameEntry->channel == 2)
        {
            CanLogCtrlData.Can2.receivedFrames++;
            m_ComputeBusLoad2(
                &CanLogCtrlData.Can2,
                pFrameEntry
            );
        }

        appCanLogStoreToFrameBuffer((void *)pFrameEntry);
        LastFrameTimestamp = pNewFrame->timestamp;
        LastFrameTimestampValid = true;

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
        CanLogManager_DrainPortEndHook();
    }
    
    if (true == CanLogCtrlData.emitSyncEntry)
    {
        uint32_t timestamp32 = LastFrameTimestampValid
            ? LastFrameTimestamp
            : CLM_ABS_TIME_TO_TIMSTAMP(AbsTime);

        CanLogCtrlData.emitSyncEntry = false;
        if (COMM_SUCCESS != CanLogManager_EmitSyncEntry(
                    &SyncEntry,
                    AbsTime,
                    timestamp32))
        {
            CanLogFileManager_ErrorHandler();
        }
    }

    if (*(CanLogCtrlData.commitLog))
    {    
        CanLogBuffer_UsedSlots((uint8_t *)&SlotsToWrite);

        if (SlotsToWrite > 0)
        {
            if (CANLOG_E_OK != CanLogBuffer_FillBlockWithPadding())
            {
                CanLogFileManager_ErrorHandler();
            }
            SdBridgeTask_Notify();
        }

#if CANLOGMANAGER_REOPEN_LOG_FILE
        if (CAN_LOG_OK != appCanLogCloseFile(data))
        {
            CanLogFileManager_ErrorHandler();
        }
#endif

#if INSTR_ENABLED
#if INSTR_PERSIST_ACTIVE
        appPersistInstrumentationData();
#endif
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
