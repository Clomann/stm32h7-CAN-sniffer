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
#include "fdcan_msg_port.h"
#include "SettingsHandler.h"
#include "CanAbs.h"
#include "fs_custom.h"
#include "timer.h"
#include "gpio.h"
#include "httpd_post.h"
#include "spi_port_freertos.h"
#include "SpiAbs.h"

#include "SettingsHandler.h"
#include "ConfigManager.h"
#include "CanLogManager.h"
#include "CanCtrl.h"

TASK_VARIABLES(CORE0_TASK0_FUNCTION, CORE0_TASK0_STACK_SIZE)
TASK_VARIABLES(CORE0_TASK1_FUNCTION, CORE0_TASK1_STACK_SIZE)
TASK_VARIABLES(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE)
TASK_VARIABLES(CORE0_TASK4_FUNCTION, CORE0_TASK4_STACK_SIZE)

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

static void CanSendTask(void *arg)
{
    static TickType_t xPreviousWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    xPreviousWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xPreviousWakeTime, xFrequency);
    
        appCanCtrlDataSetter(
            &CanCtrlData, 
            (const AppControlDataType *)&AppCtrlData, 
            (const AppConfigType *)&AppConfig);
        appFdcanPoll(&CanCtrlData);
    }
}

static void CanBridgeTask(void *arg)
{
    FDCAN_ClassicFrame Frame;
    static UBaseType_t MinUnusedStack;
    
    ( void ) MinUnusedStack;

    ulTaskNotifyTake(pdTRUE, 0);

    for (;;)
    {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        __asm volatile("nop"); 

        while (0 == CanAbs_Receive_Can1(&Frame))
        {
            fdcan_msg_port_receive(&Frame);
        }
    
        while (0 == CanAbs_Receive_Can2(&Frame))
        {
            fdcan_msg_port_receive(&Frame);
        }

        MinUnusedStack = uxTaskGetStackHighWaterMark(NULL);
    }
}

void DEFERRED_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    HAL_NVIC_ClearPendingIRQ(DEFERRED_IRQn);

    /* “Give” one notification to the bridge task */
    vTaskNotifyGiveFromISR(CanBridgeTaskHdl,
                           &xHigherPriorityTaskWoken);   /* may set it to pdTRUE */

    /* If the bridge task has a higher priority, switch to it
       immediately after exiting the ISR.  The macro name is
       port-specific: on Cortex-M it is usually portYIELD_FROM_ISR(). */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CanAbs_RxNotificationCallback()
{
    __DSB();                                    /* ensure writes complete */
    HAL_NVIC_SetPendingIRQ(DEFERRED_IRQn);  
}

void SpiAbs_TaskControlCallback(uint32_t timeout)
{
    ulTaskNotifyTake(pdTRUE, timeout);
}

void SpiAbs_TaskSendReceiveCallback()
{
    if (SpiAbs_TaskHdl != NULL)
    {
        xTaskNotifyGive(SpiAbs_TaskHdl);
    }
    else
    {
        Error_Handler();
    }
}

static void Core0Task0Main( void * parameters )
{
    uint32_t spiClockSource;
    HAL_StatusTypeDef HalStatus;
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
    
    HalStatus = spi_port_freertos_init((TaskHandle_t*)&Core0Task0MainHdl);

    if(HalStatus != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    spiClockSource = __HAL_RCC_GET_SPI1_SOURCE();
    (void)spiClockSource;

    /* USER CODE END 5 */

    /*##-2- Start the Full Duplex Communication process ########################*/
    /* While the SPI in TransmitReceive process, user can transmit data through
        "aTxBuffer" buffer & receive data through "aRxBuffer" */
    SpiAbs_PwrOn(SPIABS_DEVICE_1);

    http_init();

    run = 1U;

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

    xTaskNotifyGive(CanSendTaskHdl);

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

        if (0 == AppCtrlData.applyConfig)
        {
        }
        else if (0 == AppCtrlData.runCanTracer)
        {
            appCanCtrlDataSetter(
                &CanCtrlData, 
                (const AppControlDataType *)&AppCtrlData, 
                (const AppConfigType *)&AppConfig);
            // appCanCtrlSetBaudrate(&CanCtrlData);
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

    SpiAbs_PwrOff(SPIABS_DEVICE_1);
}

void Core0Task0Init()
{
    HAL_NVIC_SetPriority(DEFERRED_IRQn, DEFERRED_IRQ_PREEMPT_PRIO, 0);             /* 6 ≥ configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */
    HAL_NVIC_EnableIRQ(DEFERRED_IRQn);
    
    TASK_CREATE_STATIC(CORE0_TASK0_FUNCTION, CORE0_TASK0_STACK_SIZE, CORE0_TASK0_PRIO);
    TASK_CREATE_STATIC(CORE0_TASK1_FUNCTION, CORE0_TASK1_STACK_SIZE, CORE0_TASK1_PRIO);
    TASK_CREATE_STATIC(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE, CORE0_TASK2_PRIO);
    TASK_CREATE_STATIC(CORE0_TASK4_FUNCTION, CORE0_TASK4_STACK_SIZE, CORE0_TASK4_PRIO);
    
    configASSERT( CanBridgeTaskHdl != NULL );
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
