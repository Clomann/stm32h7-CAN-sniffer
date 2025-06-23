#include "Core0Task0.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "FileHandler.h"
#include "HttpAbs.h"
#include "CanLogBuffer.h"
#include "SettingsHandler.h"
#include "CanAbs.h"
#include "fs_custom.h"
#include "timer.h"
#include "gpio.h"
#include "httpd_post.h"

static StaticTask_t Core0Task0MainTCB;
static StackType_t Core0Task0MainStack[ configMINIMAL_STACK_SIZE ];


typedef struct {
  struct {
   char * filename;
    uint32_t fnamemaxlen;
    uint32_t count;
    uint32_t timestamp;
    uint8_t openRes;
    uint32_t fileHeadIndex;
    uint32_t fileTailIndex;
    bool fileIndexWrapped;
    FatFsDeviceType writeFileDevice;
  } CanLog;
  struct {
    char * filename;
    uint32_t fnamemaxlen;
    uint32_t count;
    uint32_t timestamp;
    uint8_t openRes;
    FatFsDeviceType writeFileDevice;
  } Config;
  uint8_t mountRes;
  bool runCanTracer;
  bool applyConfig;
} AppControlDataType;


static FDCAN_Message Can1TestMsg1;
static FDCAN_Message Can1TestMsg2;
static FDCAN_Message Can2TestMsg1;
static FDCAN_Message Can2TestMsg2;

uint8_t run;
static AppConfigType AppConfig;
static char CanLogFileName[255] = "/logs/CAN.LOG";
static char ConfigFileName[255] = "CONF.TXT";

static AppControlDataType AppCtrlData = { 
  .CanLog = {
    .filename = CanLogFileName,
    .fnamemaxlen = sizeof(CanLogFileName),
    .count = 0,
    .timestamp = 0,
    .openRes = 1,
    .writeFileDevice.readTargetSize = 0U,
  },
  .Config = {
    .filename = ConfigFileName,
    .fnamemaxlen = sizeof(ConfigFileName),
    .count = 0,
    .timestamp = 0,
    .openRes = 1,
    .writeFileDevice.readTargetSize = 0U,
  },
  .mountRes = 1,
  .runCanTracer = 0,
};

uint8_t Data[BLOCK_SIZE] = {0};

/* Private function prototypes -----------------------------------------------*/
void appCanCtrlSetBaudrate(uint32_t baudrate1, uint32_t baudrate2);
void appCanCtrlSetMode(uint8_t mode1, uint8_t mode2);

/* Private functions ---------------------------------------------------------*/
#define PERSIST_CAN_LOG_FILE_HEAD_TAIL 0U
#define MAX_LOG_FILE_SIZE   (8U * 1024U )
#define MAX_LOG_INDEX       (16U)

uint8_t FsCustom_GetCanLogHeadIndex(uint32_t *index)
{
    *index = AppCtrlData.CanLog.fileHeadIndex;
    return 0U;
}

uint8_t FsCustom_GetCanLogTailIndex(uint32_t *index)
{
    *index = AppCtrlData.CanLog.fileTailIndex;
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

    res = 0;
    *running = AppCtrlData.runCanTracer;

    return res;
}

void FDCAN_ErrorHandler()
{
    Error_Handler();
}

void TIM_ErrorHandler()
{
    Error_Handler();
}

void SettingsHandler_ApplyRequestCallback()
{
    AppCtrlData.applyConfig = 1;
}

static void appConfigHandlerInit(AppControlDataType *data)
{
  SettingsHandler_Init(&AppConfig);

  if ( RES_OK == data->mountRes)
  {
    data->Config.openRes = FatFS_SD_OpenFileForWrite(
                              &(data->Config.writeFileDevice),
                              data->Config.filename);

    /* enforce f_seek to zero via custom flags to not clear content later */
    data->Config.writeFileDevice.fflags = FA_CREATE_ALWAYS | FA_WRITE;
  }
  else 
  {
    data->Config.openRes = 1U;
  }
}

static int find_highest_suffix(const char *dirPath, const char *prefix, int maxSuffix)
{
    FatFS_FileIterator it;
    FILINFO *fno;
    int highest = -1;

    if ( FatFS_SD_FileIterator_Open(&it, dirPath, prefix) != FR_OK)
        return -1;

    while ( FatFS_SD_FileIterator_Next(&it, &fno) == FR_OK) {
        const char *suffix = fno->fname + strlen(prefix);
        char *endptr;
        long val = strtol(suffix, &endptr, 10);

        if (*endptr == '\0' && val >= 0 && val <= maxSuffix && val > highest) {
            highest = (int)val;
        }
    }

    FatFS_SD_FileIterator_Close(&it);
    return highest;
}

static unsigned int appCanLogOpenMostRecentFile(AppControlDataType *data)
{
    int lastUsed;

    lastUsed = find_highest_suffix("/logs/", "CAN.LOG", MAX_LOG_INDEX);     

#if 0U == PERSIST_CAN_LOG_FILE_HEAD_TAIL
    lastUsed = 0U;
#endif

    data->CanLog.fileHeadIndex = lastUsed;

    // Build candidate filename
    snprintf(data->CanLog.filename, data->CanLog.fnamemaxlen, "/logs/CAN.LOG%d", (int)lastUsed);

    data->CanLog.openRes = FatFS_SD_OpenFileForWrite(
                                    &(data->CanLog.writeFileDevice), 
                                    data->CanLog.filename);
    
    return 0U;
}

static unsigned int appCanLogCheckNewFileOpen(AppControlDataType *data)
{
    FRESULT FileSizeRes;
    uint32_t FileSize;

    FileSizeRes = FatFS_SD_GetBufferedFileSize(&(data->CanLog.writeFileDevice), &FileSize);

    if (FileSizeRes == FR_OK && FileSize >= MAX_LOG_FILE_SIZE)
    {
        // File exists and is full, advance to next one
        if (0 == FatFS_SD_CloseFile(
            &(data->CanLog.writeFileDevice)))
        {
            if (data->CanLog.fileHeadIndex >= MAX_LOG_INDEX)
            {
                data->CanLog.fileIndexWrapped = 1;
            }
            
            data->CanLog.fileHeadIndex = (data->CanLog.fileHeadIndex + 1) % (MAX_LOG_INDEX + 1);

            if (1 == data->CanLog.fileIndexWrapped)
            {
                data->CanLog.fileTailIndex = (data->CanLog.fileHeadIndex + 1) % (MAX_LOG_INDEX + 1);
            }
            
            snprintf(data->CanLog.filename, data->CanLog.fnamemaxlen, "/logs/CAN.LOG%d", (int)data->CanLog.fileHeadIndex);
    
            (void) f_unlink(data->CanLog.filename);

            data->CanLog.openRes = FatFS_SD_OpenFileForWrite(
                &(data->CanLog.writeFileDevice), 
                data->CanLog.filename);
    
            if (0 != data->CanLog.openRes)
            {
                Error_Handler();
            }
            FatFS_SD_Flush(&(data->CanLog.writeFileDevice));
        }
        else
        {            
            Error_Handler();
        }
    }
    
    return 0U;
}

static FRESULT appCanLogHandlerInit(AppControlDataType *data)
{
    FILINFO info;
    FRESULT res;

    data->CanLog.fileHeadIndex = 0;
    data->CanLog.fileTailIndex = 0;
    data->CanLog.fileIndexWrapped = 0;

    CanLogBuffer_Init();

    res = f_stat("/logs", &info);

    if ( (res == FR_OK) && (info.fattrib & AM_DIR)) 
    {

    }
    else if (res == FR_NO_FILE)
    {
        // Directory does not exist — create it
        res = f_mkdir("/logs");
        if (res != FR_OK) {
            Error_Handler();
        }
    }
    else if ( (res == FR_OK) && (!(info.fattrib & AM_DIR))) 
    {
        Error_Handler();
    }

    if ( RES_OK == data->mountRes)
    {
        appCanLogOpenMostRecentFile(data);
    }
    else 
    {
        data->CanLog.openRes = 1U;
    }

    return res;
}

static void appCanLogFillEntry(CanLogClassicCanEntryType *entry, FDCAN_ClassicFrame *frame, uint64_t *timestamp, uint8_t channel)
{
    memset(entry, 0x0, sizeof(CanLogClassicCanEntryType));

    entry->header.header_len = sizeof(entry->header);
    entry->header.type = CANLOG_CLASSIC_TYPE;
    entry->header.total_len = sizeof(CanLogClassicCanEntryType);
    entry->timestamp = *timestamp;
    entry->channel = channel;
    entry->dlc = frame->dlc;
    memcpy( (uint8_t *)&entry->can_id, (uint8_t *)&frame->id, sizeof(entry->can_id) );
    memcpy( entry->data, frame->data, sizeof(entry->data) );
}

static void appCanLogHandlerPoll(AppControlDataType *data)
{
    comm_status_t res = COMM_SUCCESS;
    bool IsOffState;
    uint32_t timestamp;
    uint8_t BlockIsReady;
    uint32_t DataLength;
    CanLogClassicCanEntryType NewEntry;
    FDCAN_ClassicFrame NewFrame;
    static volatile bool RunTracerOld = 0;

    if (RunTracerOld == AppCtrlData.runCanTracer)
    {

    }
    else if (true == AppCtrlData.runCanTracer)
    {
        CanAbs_IsStateOff_Can1(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can1())
        {
            res = COMM_ERROR;
            Error_Handler();
        }

        CanAbs_IsStateOff_Can2(&IsOffState);

        if (true == IsOffState)
        {
            /* nothing to do */
        }
        else if (COMM_SUCCESS != CanAbs_Start_Can2())
        {
            res = COMM_ERROR;
            Error_Handler();
        }

        if (COMM_SUCCESS == res)
        {
            RunTracerOld = AppCtrlData.runCanTracer;
        }
        else
        {
            AppCtrlData.runCanTracer = false;
        }
    }
    else
    {
        if (COMM_SUCCESS != CanAbs_Stop_Can1())
        {
            AppCtrlData.runCanTracer = false;
        }
        else if (COMM_SUCCESS != CanAbs_Stop_Can2())
        {
            AppCtrlData.runCanTracer = false;
        }
        else
        {
            RunTracerOld = AppCtrlData.runCanTracer;
        }
    }

    appCanLogCheckNewFileOpen(data);

    while (0 == CanAbs_Receive_Can1(&NewFrame))
    {
        timestamp = HAL_GetTick();

        appCanLogFillEntry(&NewEntry, &NewFrame, (uint64_t*)&NewFrame.timestamp, 1);

        CanLogBuffer_AddClassicCanEntry(&NewEntry);

        CanLogBuffer_IsBlockReady(&BlockIsReady);

        if ( 0 != data->CanLog.openRes )
        {    
            /* quit */
            Error_Handler();
        }
        else if ( 0 == BlockIsReady )
        {
            /* quit since block is not ready to be written */
        }
        else if ( CANLOG_E_OK !=  CanLogBuffer_ReadNextBlock(Data, &DataLength) )
        {
            /* quit since data could not be read */
        }
        else if (FR_OK == FatFS_SD_WriteFile(
                &(data->CanLog.writeFileDevice), 
                (const char *)Data, 
                DataLength) )
        {
            FatFS_SD_Flush(&(data->CanLog.writeFileDevice));
            data->CanLog.timestamp =  timestamp;
        }
        else
        {
            data->CanLog.timestamp =  timestamp;
        }
    }

    while (0 == CanAbs_Receive_Can2(&NewFrame))
    {
        timestamp = HAL_GetTick();

        appCanLogFillEntry(&NewEntry, &NewFrame, (uint64_t*)&NewFrame.timestamp, 2);

        CanLogBuffer_AddClassicCanEntry(&NewEntry);

        CanLogBuffer_IsBlockReady(&BlockIsReady);

        if ( 0 != data->CanLog.openRes )
        {    
            /* quit */
            Error_Handler();
        }
        else if ( 0 == BlockIsReady )
        {
            /* quit since block is not ready to be written */
        }
        else if ( CANLOG_E_OK !=  CanLogBuffer_ReadNextBlock(Data, &DataLength) )
        {
            /* quit since data could not be read */
        }
        else if (FR_OK == FatFS_SD_WriteFile(
                &(data->CanLog.writeFileDevice), 
                (const char *)Data, 
                DataLength) )
        {
            FatFS_SD_Flush(&(data->CanLog.writeFileDevice));
            data->CanLog.timestamp =  timestamp;
        }
        else
        {
            data->CanLog.timestamp =  timestamp;
        }
    }
}

static void appCanLogHandlerDeInit(AppControlDataType * data)
{
  if ( 0 == data->CanLog.openRes )
  { 
    FatFS_SD_CloseFile(&(data->CanLog.writeFileDevice));
  }
}

static void appFdcanInit()
{
    static uint8_t TxData[8];
    static uint8_t TxData2[8];
    
    memset(TxData, 0xFF, sizeof(TxData));
    memset(TxData2, 0xFF, sizeof(TxData2));

    /* Initialize FDCAN timestamp external timer */
    if (0 != TIMx_Init(TIMx_TIME_RESOLUTION) )
    {
        Error_Handler();
    }

    if ( 0 == CanAbs_Init_Can1(AppConfig.can1.baudrate) ) 
    {
        CanAbs_CreateMessage_Standard(&Can1TestMsg1, 0x321, &TxData[0], sizeof(TxData) / sizeof(*TxData));
        CanAbs_CreateMessage_Standard(&Can1TestMsg2, 0x322, &TxData[0], sizeof(TxData) / sizeof(*TxData));
    }
    else
    {
        Error_Handler();
    }

    if ( 0 == CanAbs_Init_Can2(AppConfig.can2.baudrate) ) 
    {
        CanAbs_CreateMessage_Standard(&Can2TestMsg1, 0x323, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
        CanAbs_CreateMessage_Standard(&Can2TestMsg2, 0x324, &TxData2[0], sizeof(TxData2) / sizeof(*TxData2));
    }
    else
    {
        Error_Handler();
    }
}

static void appFdcanPoll()
{
    if ( 0 != CanAbs_Send_Can1(&Can1TestMsg1))
    {
        Error_Handler();
    }

    if ( 0 != CanAbs_Send_Can1(&Can1TestMsg2))
    {
        Error_Handler();
    }

    if ( 0 != CanAbs_Send_Can2(&Can2TestMsg1))
    {
        Error_Handler();
    }

    if ( 0 != CanAbs_Send_Can2(&Can2TestMsg2))
    {
        Error_Handler();
    }
}

static void Core0Task0Main( void * parameters )
{
    static uint32_t timestamp_prev = 0U;
    uint32_t timestamp = 0U;
    uint32_t time_delta = 0U;
    static FatFsDeviceType ConfigReadFileDevice;
    uint32_t spiClockSource;
    HAL_StatusTypeDef HalStatus;
    struct Config { 
        char data[1024U];
        uint32_t len;
    } Config = {0U};
    
    /* Unused parameters. */
    ( void ) parameters;

    /* Initialize HAL SysTick external timer */
    if (0 != TIM_HAL_Init(TIM_HAL_TIME_FREQ) )
    {
        Error_Handler();
    }

    HalStatus = SPI_Init();

    if(HalStatus != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    spiClockSource = __HAL_RCC_GET_SPI1_SOURCE();
    (void)spiClockSource;

    /* Infinite loop */
    while (1)
    {
        /* USER CODE END 5 */

        /*##-2- Start the Full Duplex Communication process ########################*/
        /* While the SPI in TransmitReceive process, user can transmit data through
            "aTxBuffer" buffer & receive data through "aRxBuffer" */
        Spi_PwrOn();
        if (0U == FatFS_SD_LoadConfig(&ConfigReadFileDevice, Config.data, &Config.len) )
        {
            SettingsHandler_ParseConfig(Config.data, Config.len, &AppConfig);
            SettingsHandler_Init(&AppConfig);
        }

        http_init();

        run = 1U;

        if (0 != AppCtrlData.mountRes)
        AppCtrlData.mountRes = FatFS_SD_Mount();

        appCanLogHandlerInit(&AppCtrlData);

        appConfigHandlerInit(&AppCtrlData);

        GPIO_Dbg_Init();
        GPIO_Mco1_Init();
        
        appFdcanInit();

        while (run)
        {
            timestamp = HAL_GetTick() * HAL_GetTickFreq();
            time_delta = timestamp - timestamp_prev;

            if ( time_delta < 10 )
            {

            }
            else if (0 == AppCtrlData.runCanTracer)
            {
                
            }
            else
            {
                appFdcanPoll();
                timestamp_prev = timestamp;
            }

            http_poll();

            appCanLogHandlerPoll(&AppCtrlData);

            if (0 == AppCtrlData.Config.openRes)
            {
                SettingsHandler_Poll(&(AppCtrlData.Config.writeFileDevice), &AppConfig);
            }

            if (0 == AppCtrlData.applyConfig)
            {
            }
            else if (0 == AppCtrlData.runCanTracer)
            {
                appCanCtrlSetBaudrate(AppConfig.can1.baudrate, AppConfig.can2.baudrate);
                appCanCtrlSetMode(AppConfig.can1.mode, AppConfig.can2.mode);
                AppCtrlData.applyConfig = 0;
            }
            else
            {
                AppCtrlData.applyConfig = 0;
            }
            
        }

        appCanLogHandlerDeInit(&AppCtrlData);

        if (0 == AppCtrlData.mountRes)
        FatFS_SD_Unmount();

        Spi_PwrOff();
    }
}

void Core0Task0Init()
{
    ( void ) xTaskCreateStatic( Core0Task0Main,
                                "example",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( Core0Task0MainStack[ 0 ] ),
                                &( Core0Task0MainTCB ) );
}

/*!< Time in micro seconds */
static volatile uint64_t Time;

void TIM_InterruptCallback()
{
    static uint64_t Arr;

    TIM_GetArrValue((uint16_t*)&Arr);
    Time += Arr * TIMx_TIME_RESOLUTION;
}

void TIM_HAL_InterruptCallback()
{
    HAL_IncTick();
    GPIO_Dbg_Toggle();
}

/**
 * 
 * \param[out] timestamp in micro seconds.
 */
comm_status_t FDCAN_GetTimestamp(uint64_t *timestamp)
{
    comm_status_t res;
    uint64_t time_snapshot1, time_snapshot2;
    uint16_t cnt;

    res = COMM_SUCCESS;

    do {
        time_snapshot1 = Time;
        TIM_GetCounterValue(&cnt);
        time_snapshot2 = Time;
    } while (time_snapshot1 != time_snapshot2);

    *timestamp = time_snapshot1 + (uint64_t)(cnt * TIMx_TIME_RESOLUTION);

    return res;
}

void appCanCtrlSetBaudrate(uint32_t baudrate1, uint32_t baudrate2)
{
    
    if (COMM_SUCCESS == CanAbs_SetBaudrate_Can1(baudrate1))
    {
        Error_Handler();
    }

    if (COMM_SUCCESS == CanAbs_SetBaudrate_Can2(baudrate2))
    {
        Error_Handler();
    }
}

void appCanCtrlSetMode(uint8_t mode1, uint8_t mode2)
{
    if (COMM_SUCCESS == CanAbs_SetMode_Can1(mode1))
    {
        Error_Handler();
    }

    if (COMM_SUCCESS == CanAbs_SetMode_Can2(mode2))
    {
        Error_Handler();
    }
}

void appCtrlCgiHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint32_t i = 0;
    char * param = NULL;
    char * value = NULL;

    if (iIndex==0)
    {
        /* Check cgi parameter */
        for (i = 0; i<(uint32_t)iNumParams; i++)
        {
            param = pcParam[i];
            value = pcValue[i];

            /* check parameter "baudrate" */
            if (strcmp(param , "action") == 0)
            {
                if(strcmp(value, "Stop") == 0)
                {
                    AppCtrlData.runCanTracer = 0;
                }
                else if(strcmp(value, "Start") == 0)
                {
                    AppCtrlData.runCanTracer = 1;
                }
            }
        }
    }
}
