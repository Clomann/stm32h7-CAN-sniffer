#include "Core0Task0.h"
#include "Core0TasksCfg.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "nvic_irg_config.h"

#include "FileHandler.h"
#include "HttpAbs.h"
#include "SettingsHandler.h"
#include "CanAbs.h"
#include "fs_custom.h"
#include "timer.h"
#include "gpio.h"
#include "httpd_post.h"

#include "CanBridgeTask.h"
#include "CanSendTask.h"
#include "SpiTask.h"

#include "SettingsHandler.h"
#include "ConfigManager.h"
#include "CanLogManager.h"
#include "CanCtrl.h"
#include "WebInterface.h"

TASK_VARIABLES(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE)

typedef struct {
    CanLogControlDataType *Log;
    ConfigManagerType Config;
    uint8_t mountRes;
    bool runCanTracer;
    bool applyConfig;
} AppControlDataType;

uint8_t run;
static AppConfigType AppConfig;
static CanCtrlDataType CanCtrlData;

static AppControlDataType AppCtrlData = { 
    .mountRes = 1,
    .runCanTracer = 0
};

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
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

void SettingsHandler_ApplyRequestCallback()
{
    AppCtrlData.applyConfig = 1;
}

void vApplicationStackOverflowHook( TaskHandle_t t, char *name )
{
    ( void ) name;
    taskDISABLE_INTERRUPTS();
    __BKPT(1);                     /* hit here => stack overflow      */
}

static void appCanCtrlDataSetter(
    CanCtrlDataType * data, 
    const AppControlDataType *appData, 
    const AppConfigType * appConfig)
{
    CanCtrlData.sendingActive = appData->runCanTracer; 
    CanCtrlData.can1.baudrate = AppConfig.can1.baudrate;
    CanCtrlData.can1.mode = AppConfig.can1.mode;
    CanCtrlData.can2.baudrate = AppConfig.can2.baudrate;
    CanCtrlData.can2.mode = AppConfig.can2.mode;
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
    SpiTask_PortInit((TaskHandle_t*)&Core0Task0MainHdl);

    http_init();

    if (RES_OK != AppCtrlData.mountRes)
    {
        AppCtrlData.mountRes = FatFS_SD_Mount();
    }

    AppCtrlData.Log = CanLogHandler_Init(&AppCtrlData.mountRes, &AppCtrlData.runCanTracer);
    appCanLogHandlerInit(AppCtrlData.Log);
    
    ConfigManager_Init(&AppCtrlData.Config, "CONF.TXT", &AppCtrlData.mountRes);
    if (ConfigManager_Initialize(&AppCtrlData.Config) == CONFIG_OK)
    {
        if (ConfigManager_LoadConfig(&AppCtrlData.Config, &AppConfig) != CONFIG_OK) 
        {
            AppConfig.can1.baudrate = 0;
            AppConfig.can1.mode = 0;
            AppConfig.can2.baudrate = 0;
            AppConfig.can2.mode = 0;
        }

        SettingsHandler_Init(&AppConfig);
    }

    GPIO_Dbg_Init();
    GPIO_Mco1_Init();

    appCanCtrlDataSetter(
        &CanCtrlData, 
        (const AppControlDataType *)&AppCtrlData, 
        (const AppConfigType *)&AppConfig);    
    appFdcanInit(&CanCtrlData);

    /* Release CanSendTask */
    CanSendTask_Notify();

    run = 1U;
    
    while (run)
    {
        http_poll();

        appCanLogHandlerPoll(AppCtrlData.Log);

        if (SettingsHandler_Poll(&AppConfig)) 
        {
            if (ConfigManager_UpdateConfig(&AppCtrlData.Config, &AppConfig, true) == CONFIG_OK) 
            {
                AppConfig.updated = 0;
            }
        }

        CanSendTask_SetSendingActive(AppCtrlData.runCanTracer);

        if (0 == AppCtrlData.applyConfig)
        {
        }
        else if (0 == AppCtrlData.runCanTracer)
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
