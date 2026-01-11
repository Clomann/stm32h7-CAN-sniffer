#include "Core0Task0.h"
#include "Core0TasksCfg.h"
#include "TasksHooks.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "main.h"
#include "profiling.h"
#include "instrumentation.h"

#include "nvic_irg_config.h"

#include "FileHandler.h"
#include "HttpAbs.h"
#include "SettingsHandler.h"
#include "CanAbs.h"
#include "fdcan.h"
#include "fs_custom.h"
#include "timer.h"
#include "gpio.h"
#include "httpd_post.h"
#include "core_json.h"

#include "CanBridgeTask.h"
#include "CanSendTask.h"
#include "Core0Task1.h"
#include "SpiTask.h"
#include "SdBridgeTask.h"

#include "SettingsHandler.h"
#include "ConfigManager.h"
#include "CanLogManager.h"
#include "CanCtrl.h"
#include "WebInterface.h"
#include "RuntimeChecks.h"

TASK_VARIABLES(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE)

typedef struct {
    CanLogControlDataType *Log;
    ConfigManagerType Config;
    uint8_t mountRes;
    bool runCanTracer;
    bool applyConfig;
    bool commitLog;
} AppControlDataType;

uint8_t run;
static AppConfigType AppConfig;
static CanCtrlDataType CanCtrlData;

static AppControlDataType AppCtrlData = { 
    .mountRes = 1,
    .runCanTracer = 0,
    .commitLog = 0
};

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void  CanAbs_ErrorHandler(void)
{
    Error_Handler();
}

void SpiTask_ErrorHandlerHook(void)
{
    Error_Handler();
}

void CANCONTROL_ErrorHandlerHook()
{
    Error_Handler();
}

void CanLogFileManager_ErrorHandler()
{
    Error_Handler();
}

void FDCAN_ErrorHandler()
{
    Error_Handler();
}

void TIM_ErrorHandler()
{
    Error_Handler();
}

void Sd_Spi_ErrorHandlerHook(ErrorContextType *context)
{
    Error_Handler();
}

#if INSTR_ENABLED
void Instrumentation_ErrorHandlerHook(ErrorContextType *context)
{
    (void) context;

    Error_Handler();
}
#endif 

void SettingsHandler_ApplyRequestCallback()
{
    AppCtrlData.applyConfig = 1;
}

void vApplicationStackOverflowHook( TaskHandle_t t, char *name )
{
    (void) (name);
    (void) (t);
    
    taskDISABLE_INTERRUPTS();
    __BKPT(1);                     /* hit here => stack overflow      */
}

static void appHandleFormattingRequest(void)
{
    FRESULT res;
    _Bool ReformattingRequested;
    FatFsDeviceType DevTmp;
    const char FormatRequestFileName[] = FILEHANDLER_FORMATTING_REQUEST_FILENAME;
    uint32_t cluster_size = CLUSTER_SIZE;
    uint32_t log_file_size = MAX_LOG_FILE_SIZE;
    uint32_t log_file_count = MAX_LOG_FILE_COUNT;

    res = FatFS_SD_OpenFileForRead(&DevTmp, FormatRequestFileName);

    if (FR_OK == res)
    {
        ReformattingRequested = true;

        do
        {
            uint32_t fileSize = 0U;
            char buffer[128];
            uint32_t readSize = 0U;
            JSONStatus_t result;
            char *value = NULL;
            size_t valueLength = 0U;
            char tmp[32];
            uint32_t parsed = 0U;

            res = FatFS_SD_GetFileSize(&DevTmp, &fileSize);
            if (FR_OK != res || fileSize == 0U)
            {
                break;
            }

            readSize = (fileSize < (sizeof(buffer) - 1U)) ? fileSize : (sizeof(buffer) - 1U);
            res = FatFS_SD_ReadFile(&DevTmp, buffer, readSize);
            if (FR_OK != res)
            {
                break;
            }

            buffer[readSize] = '\0';

            result = JSON_Validate(buffer, readSize);
            if (JSONSuccess != result)
            {
                break;
            }

            result = FileHandler_GetValue(
                buffer,
                readSize,
                "cluster_size",
                sizeof("cluster_size") - 1U,
                &value,
                &valueLength
            );
            if (JSONSuccess == result && valueLength < sizeof(tmp))
            {
                memcpy(tmp, value, valueLength);
                tmp[valueLength] = '\0';
                if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U))
                {
                    cluster_size = parsed;
                }
            }

            result = FileHandler_GetValue(
                buffer,
                readSize,
                "log_file_size",
                sizeof("log_file_size") - 1U,
                &value,
                &valueLength
            );
            if (JSONSuccess == result && valueLength < sizeof(tmp))
            {
                memcpy(tmp, value, valueLength);
                tmp[valueLength] = '\0';
                if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U))
                {
                    log_file_size = parsed;
                }
            }

            result = FileHandler_GetValue(
                buffer,
                readSize,
                "log_file_count",
                sizeof("log_file_count") - 1U,
                &value,
                &valueLength
            );
            if (JSONSuccess == result && valueLength < sizeof(tmp))
            {
                memcpy(tmp, value, valueLength);
                tmp[valueLength] = '\0';
                if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U))
                {
                    log_file_count = parsed;
                }
            }
        } while (0);

        (void)FatFS_SD_CloseFile(&DevTmp);
    }
    else
    {
        ReformattingRequested = false;
    }
    
    if (ReformattingRequested)
    {
        res = FatFS_SD_Unmount();

        if (FR_OK == res)
        {
            AppCtrlData.mountRes = 1;
    
            res = FatFS_SD_Format_Fat32(cluster_size);
        }

        if (FR_OK == res)
        {
            AppCtrlData.mountRes = FatFS_SD_Mount();
        }
    }

    (void)log_file_size;
    (void)log_file_count;
}

static void appCanCtrlDataSetter(
    CanCtrlDataType * data, 
    const AppControlDataType *appData, 
    const AppConfigType * appConfig)
{
    data->sendingActive = appData->runCanTracer; 
    data->can1.baudrate = appConfig->can1.baudrate;
    data->can1.mode = appConfig->can1.mode;
    data->can2.baudrate = appConfig->can2.baudrate;
    data->can2.mode = appConfig->can2.mode;
}

static void appConfigSetDefaults(AppConfigType *config)
{
    config->can1.baudrate = FDCAN_BAUDRATE_250000;
    config->can1.mode = FDCAN_MODE_2;
    config->can2.baudrate = FDCAN_BAUDRATE_250000;
    config->can2.mode = FDCAN_MODE_2;
}

static void Core0Task0Main( void * parameters )
{
    static UBaseType_t MinUnusedStack;

    /* Unused parameters. */
    ( void ) parameters;
    ( void ) MinUnusedStack;

    // Mmc_Init();

    /* Initialize HAL SysTick external timer */
    if (0 != TIM_HAL_Init(TIM_HAL_TIME_FREQ) )
    {
        Error_Handler();
    }

    /* initialialize port early to allow for taskless SPI communication */
    SpiTask_PortInit();

    http_init();
    
    if (RES_OK != AppCtrlData.mountRes)
    {
        AppCtrlData.mountRes = FatFS_SD_Mount();
    }
    
    if (FR_OK != AppCtrlData.mountRes)
    {
        Error_Handler();
    }

    appHandleFormattingRequest();

    AppCtrlData.Log = CanLogHandler_Init(
        &AppCtrlData.mountRes, 
        &AppCtrlData.runCanTracer, 
        &AppCtrlData.commitLog);
    appCanLogHandlerInit(AppCtrlData.Log);
    Core0Task1_SetCanLogHandle(AppCtrlData.Log);

    appConfigSetDefaults(&AppConfig);

    appCanCtrlDataSetter(
        &CanCtrlData, 
        (const AppControlDataType *)&AppCtrlData, 
        (const AppConfigType *)&AppConfig);    
    appFdcanInit(&CanCtrlData);

    ConfigManager_Init(&AppCtrlData.Config, "CONF.TXT", &AppCtrlData.mountRes);
    if (ConfigManager_Initialize(&AppCtrlData.Config) == CONFIG_OK)
    {
        if (ConfigManager_LoadConfig(&AppCtrlData.Config, &AppConfig) != CONFIG_OK) 
        {
            appConfigSetDefaults(&AppConfig);
        }
        else 
        {
            appCanCtrlDataSetter(
                &CanCtrlData, 
                (const AppControlDataType *)&AppCtrlData, 
                (const AppConfigType *)&AppConfig);
            appCanCtrlSetBaudrate(&CanCtrlData);
            appCanCtrlSetMode(&CanCtrlData);
            
            appCanLogSetParam(CLM_PARAMETER_ID_CAN1_BAUDRATE, AppConfig.can1.baudrate);
            appCanLogSetParam(CLM_PARAMETER_ID_CAN2_BAUDRATE, AppConfig.can2.baudrate);
        }

        SettingsHandler_Init(&AppConfig);
    }

    /* Release CanSendTask */
    CanSendTask_Notify();

    run = 1U;
    
    while (run)
    {
        http_poll();

        if (SettingsHandler_Poll(&AppConfig)) 
        {
            if (ConfigManager_UpdateConfig(&AppCtrlData.Config, &AppConfig, true) == CONFIG_OK) 
            {
                appCanLogSetParam(CLM_PARAMETER_ID_CAN1_BAUDRATE, AppConfig.can1.baudrate);
                appCanLogSetParam(CLM_PARAMETER_ID_CAN2_BAUDRATE, AppConfig.can2.baudrate);
            }
            else
            {
                Error_Handler();
            }
            
            AppConfig.updated = 0;
        }

        CanSendTask_SetSendingActive(AppCtrlData.runCanTracer);

        if (0 != AppCtrlData.applyConfig && 0 == AppCtrlData.runCanTracer)
        {
            appCanCtrlDataSetter(
                &CanCtrlData, 
                (const AppControlDataType *)&AppCtrlData, 
                (const AppConfigType *)&AppConfig);
            appCanCtrlSetBaudrate(&CanCtrlData);
            appCanCtrlSetMode(&CanCtrlData);
            AppCtrlData.applyConfig = 0;
        }
        else
        {
            AppCtrlData.applyConfig = 0;
        }

        MinUnusedStack = uxTaskGetStackHighWaterMark(NULL);

        if (MinUnusedStack < 50)
        {
            Tasks_ErrorHandler();
        }

        update_task_stats();
    }

    appCanLogHandlerDeInit(AppCtrlData.Log);

    if (RES_OK == AppCtrlData.mountRes)
    {
        FatFS_SD_Unmount();
        AppCtrlData.mountRes = RES_NOTRDY;
    }

    SpiAbs_PortDeInit();
}

void Core0Task0Init()
{
    CanBridgeTaskInit();

    CanSendTaskInit();

    SpiTask_Init();

    SdBridgeTask_Init();

    force_profiling_link();
    
    TASK_CREATE_STATIC(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE, CORE0_TASK2_PRIO);
}

/* Hooks */

int ConfigManager_SerializeHook(const void* config, char* buffer, uint32_t maxLength, uint32_t* length) 
{
    if (!config || !buffer || !length) 
    {
        return -1;
    }
    
    return SettingsHandler_CreateJsonString((AppConfigType*)config, buffer, maxLength, length);
}

int ConfigManager_DeserializeHook(const char* buffer, uint32_t length, void* config) 
{
    if (!buffer || !config) 
    {
        return -1;
    }
    
    if (SETTINGS_OK == SettingsHandler_ParseConfig((char*)buffer, length, (AppConfigType*)config))
    {
        return CONFIG_OK;
    }
    else
    {
        return CONFIG_NOT_OK;
    }
}

void WebInterface_GetActionHook(uint8_t action)
{
    AppCtrlData.runCanTracer = action;
}

void WebInterface_RequestFormattingHook(
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    FatFS_SD_Formatting_Request(cluster_size, log_file_size, log_file_count);
}
