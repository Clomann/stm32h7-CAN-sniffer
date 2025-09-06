#include <string.h>
#include <stdio.h>

#include "CanLogManager.h"

#include "fs_custom.h"
#include "CanLogBuffer.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"

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
};

static char CanLogFileName[255] = "/logs/CAN.LOG";
static CanLogControlDataType CanLogCtrlData;

static int
find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix);
static unsigned int appCanLogOpenMostRecentFile(CanLogControlDataType *data);
static unsigned int appCanLogCheckNewFileOpen(CanLogControlDataType *data);
static void appCanLogFillEntry(
    CanLogClassicCanEntryType *entry,
    FDCAN_ClassicFrame *frame,
    uint64_t *timestamp,
    uint8_t channel
);
static comm_status_t
appCanLogStoreToFrameBuffer(FDCAN_ClassicFrame *frame, uint8_t channel);
static comm_status_t
appCanLogStoreToSd(FatFsDeviceType *dev, char *data, uint32_t length);
static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev);

void __attribute__((weak)) CanLogFileManager_ErrorHandler()
{
    ;
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
    *capacity = MAX_LOG_INDEX;
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
        "/logs/CAN.LOG%d",
        (int)lastUsed
    );

    CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        CanLogCtrlData.CanLog.filename
    );

    return 0U;
}

static unsigned int appCanLogCheckNewFileOpen(CanLogControlDataType *data)
{
    FRESULT FileSizeRes;
    uint32_t FileSize;

    (void)data;

    FileSizeRes = FatFS_SD_GetBufferedFileSize(
        &(CanLogCtrlData.CanLog.writeFileDevice),
        &FileSize
    );

    if (FileSizeRes == FR_OK && FileSize >= MAX_LOG_FILE_SIZE)
    {
        if (FR_OK != FatFS_SD_Flush(&(CanLogCtrlData.CanLog.writeFileDevice)))
        {
            CanLogFileManager_ErrorHandler();
        }

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

            (void)f_unlink(CanLogCtrlData.CanLog.filename);

            CanLogCtrlData.CanLog.openRes = FatFS_SD_OpenFileForWrite(
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

CanLogControlDataType *CanLogHandler_Init(uint8_t *mount_res, bool *run)
{
    memset(&CanLogCtrlData, 0x0, sizeof(CanLogCtrlData));

    CanLogCtrlData.CanLog.filename    = CanLogFileName;
    CanLogCtrlData.CanLog.fnamemaxlen = sizeof(CanLogFileName);
    CanLogCtrlData.CanLog.openRes     = 1;

    CanLogCtrlData.mountRes     = mount_res;
    CanLogCtrlData.runCanTracer = run;

    return &CanLogCtrlData;
}

FRESULT appCanLogHandlerInit(CanLogControlDataType *data)
{
    FILINFO info;
    FRESULT res;

    *CanLogCtrlData.runCanTracer           = false;
    CanLogCtrlData.runCanTracerOld         = false;
    CanLogCtrlData.CanLog.fileHeadIndex    = 0;
    CanLogCtrlData.CanLog.fileTailIndex    = 0;
    CanLogCtrlData.CanLog.fileIndexWrapped = 0;

    CanLogBuffer_Init();

    fdcan_msg_port_init();

    res = f_stat("/logs", &info);

    if ((res == FR_OK) && (info.fattrib & AM_DIR))
    {
    }
    else if (res == FR_NO_FILE)
    {
        // Directory does not exist — create it
        res = f_mkdir("/logs");
        if (res != FR_OK)
        {
            CanLogFileManager_ErrorHandler();
        }
    }
    else if ((res == FR_OK) && (!(info.fattrib & AM_DIR)))
    {
        CanLogFileManager_ErrorHandler();
    }

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
    uint64_t *timestamp,
    uint8_t channel
)
{
    memset(entry, 0x0, sizeof(CanLogClassicCanEntryType));

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CANLOG_CLASSIC_TYPE;
    entry->header.total_len  = sizeof(CanLogClassicCanEntryType);
    entry->timestamp         = *timestamp;
    entry->channel           = channel;
    entry->dlc               = frame->dlc;
    memcpy(
        (uint8_t *)&entry->can_id,
        (uint8_t *)&frame->id,
        sizeof(entry->can_id)
    );
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
        (uint64_t *)&frame->timestamp,
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

    return res;
}

static comm_status_t appCanLogStoreBlock(FatFsDeviceType *dev)
{
    uint32_t DataLength;
    comm_status_t res               = COMM_SUCCESS;
    static uint8_t Data[BLOCK_SIZE] = {0};

    if (CANLOG_E_OK == CanLogBuffer_ReadNextBlock(Data, &DataLength))
    {
        res = appCanLogStoreToSd(dev, (char *)Data, DataLength);
    }
    else
    {
        res = COMM_ERROR;
    }

    return res;
}

void appCanLogHandlerPoll(CanLogControlDataType *data)
{
    comm_status_t res = COMM_SUCCESS;
    bool IsOffState;
    uint8_t BlockIsReady;
    FDCAN_ClassicFrame NewFrame;

    if (CanLogCtrlData.runCanTracerOld == *CanLogCtrlData.runCanTracer)
    {
    }
    else if (true == *CanLogCtrlData.runCanTracer)
    {
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
    }

    appCanLogCheckNewFileOpen(data);

    while (0 < fdcan_msg_port_read(&NewFrame, 0))
    {
        appCanLogStoreToFrameBuffer(&NewFrame, NewFrame.channel);

        CanLogBuffer_IsBlockReady(&BlockIsReady);

        if (0 != CanLogCtrlData.CanLog.openRes)
        {
            /* quit */
            CanLogFileManager_ErrorHandler();
        }
        else if (0 < BlockIsReady)
        {
            appCanLogStoreBlock(&(CanLogCtrlData.CanLog.writeFileDevice));
        }
        else
        {
        }
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
