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


static FDCAN_Message Can1TestMsg1;
static FDCAN_Message Can1TestMsg2;
static FDCAN_Message Can2TestMsg1;
static FDCAN_Message Can2TestMsg2;

uint8_t run;
static AppConfigType AppConfig;

static AppControlDataType AppCtrlData = { 
    .mountRes = 1,
    .runCanTracer = 0
};

/* Private function prototypes -----------------------------------------------*/
void appCanCtrlSetBaudrate(uint32_t baudrate1, uint32_t baudrate2);
void appCanCtrlSetMode(uint8_t mode1, uint8_t mode2);

static void appFdcanPoll();

/* Private functions ---------------------------------------------------------*/
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

static void CanSendTask(void *arg)
{
    static TickType_t xPreviousWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    xPreviousWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xPreviousWakeTime, xFrequency);
        
        appFdcanPoll();
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

void appFdcanPoll()
{
    volatile uint8_t res;
    
    res = CanAbs_Send_Can1(&Can1TestMsg1);
    if ( 0 != res)
    {
        Error_Handler();
    }

    res = CanAbs_Send_Can1(&Can1TestMsg2);
    if ( 0 != res)
    {
        Error_Handler();
    }

    res = CanAbs_Send_Can2(&Can2TestMsg1);
    if ( 0 != res)
    {
        Error_Handler();
    }

    res = CanAbs_Send_Can2(&Can2TestMsg2);
    if ( 0 != res)
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
    
    appFdcanInit();

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
            appCanCtrlSetBaudrate(AppConfig.can1.baudrate, AppConfig.can2.baudrate);
            appCanCtrlSetMode(AppConfig.can1.mode, AppConfig.can2.mode);
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

/*!< Time in micro seconds */
static volatile uint64_t Time = 0;

void TIM_InterruptCallback()
{
    static uint64_t Arr = 0;

    TIM_GetArrValue((uint16_t*)&Arr);
    Time += Arr * TIMx_TIME_RESOLUTION;
}

void TIM_HAL_InterruptCallback()
{
    HAL_IncTick();
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
