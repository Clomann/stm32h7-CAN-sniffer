#include "Core0Task0.h"
#include "Core0TasksCfg.h"
#include "TasksHooks.h"

#include <stdint.h>
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
#include "Iap.h"
#include "FwUpdateHandoff.h"

extern uint8_t __scratch_end__;

TASK_VARIABLES(CORE0_TASK2_FUNCTION, CORE0_TASK2_STACK_SIZE)

typedef struct
{
    CanLogControlDataType *Log;
    ConfigManagerType Config;
    uint8_t mountRes;
    bool runCanTracer;
    bool applyConfig;
    bool applyFirmwareUpdate;
    bool commitLog;
} AppControlDataType;

uint8_t run;
static AppConfigType AppConfig;
static CanCtrlDataType CanCtrlData;

static AppControlDataType AppCtrlData = {
    .mountRes            = 1,
    .runCanTracer        = 0,
    .applyFirmwareUpdate = 0,
    .commitLog           = 0
};

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void CanAbs_ErrorHandler(void)
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

void FileHandler_ErrorHandler(ErrorContextType *context)
{
    Error_Handler();
}

#if INSTR_ENABLED
void Instrumentation_ErrorHandlerHook(ErrorContextType *context)
{
    (void)context;

    Error_Handler();
}
#endif

void SettingsHandler_ApplyRequestCallback()
{
    AppCtrlData.applyConfig = 1;
}

void vApplicationStackOverflowHook(TaskHandle_t t, char *name)
{
    (void)(name);
    (void)(t);

    taskDISABLE_INTERRUPTS();
    __BKPT(1); /* hit here => stack overflow      */
}

static bool appParseLogConfigJson(
    char *buffer,
    uint32_t buffer_len,
    uint32_t *cluster_size,
    uint32_t *log_file_size,
    uint32_t *log_file_count
)
{
    JSONStatus_t result;
    char *value        = NULL;
    size_t valueLength = 0U;
    char tmp[32];
    uint32_t parsed = 0U;

    result = JSON_Validate(buffer, buffer_len);
    if (JSONSuccess != result)
    {
        return false;
    }

    result = FileHandler_GetValue(
        buffer,
        buffer_len,
        "cluster_size",
        sizeof("cluster_size") - 1U,
        &value,
        &valueLength
    );
    if (JSONSuccess == result && valueLength < sizeof(tmp))
    {
        memcpy(tmp, value, valueLength);
        tmp[valueLength] = '\0';
        if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U)
            && parsed > 0U)
        {
            *cluster_size = parsed;
        }
    }

    result = FileHandler_GetValue(
        buffer,
        buffer_len,
        "log_file_size",
        sizeof("log_file_size") - 1U,
        &value,
        &valueLength
    );
    if (JSONSuccess == result && valueLength < sizeof(tmp))
    {
        memcpy(tmp, value, valueLength);
        tmp[valueLength] = '\0';
        if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U)
            && parsed > 0U)
        {
            *log_file_size = parsed;
        }
    }

    result = FileHandler_GetValue(
        buffer,
        buffer_len,
        "log_file_count",
        sizeof("log_file_count") - 1U,
        &value,
        &valueLength
    );
    if (JSONSuccess == result && valueLength < sizeof(tmp))
    {
        memcpy(tmp, value, valueLength);
        tmp[valueLength] = '\0';
        if (0U == FileHandler_ConvertToInteger(tmp, &parsed, 10U)
            && parsed > 0U)
        {
            *log_file_count = parsed;
        }
    }

    return true;
}

static bool appLoadLogConfigFile(
    const char *filename,
    uint32_t *cluster_size,
    uint32_t *log_file_size,
    uint32_t *log_file_count
)
{
    FRESULT res;
    FatFsDeviceType DevTmp;
    uint32_t fileSize = 0U;
    char buffer[128];
    uint32_t readSize = 0U;
    bool parsed       = false;

    res = FatFS_SD_OpenFileForRead(&DevTmp, filename);

    if (FR_OK == res)
    {
        do
        {
            res = FatFS_SD_GetFileSize(&DevTmp, &fileSize);
            if (FR_OK != res || fileSize == 0U)
            {
                break;
            }

            readSize = (fileSize < (sizeof(buffer) - 1U))
                           ? fileSize
                           : (sizeof(buffer) - 1U);
            res      = FatFS_SD_ReadFile(&DevTmp, buffer, readSize);
            if (FR_OK != res)
            {
                break;
            }

            buffer[readSize] = '\0';
            parsed           = appParseLogConfigJson(
                buffer,
                readSize,
                cluster_size,
                log_file_size,
                log_file_count
            );
        } while (0);

        (void)FatFS_SD_CloseFile(&DevTmp);
    }

    return parsed;
}

static FRESULT appStoreLogConfigFile(
    const char *filename,
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
)
{
    FRESULT res;
    FatFsDeviceType File;
    char content[128];
    int length;

    res = FatFS_SD_OpenFileForOverWrite(&File, filename);

    if (res != FR_OK)
    {
        return res;
    }

    length = snprintf(
        content,
        sizeof(content),
        "{\"cluster_size\":%lu,\"log_file_size\":%lu,\"log_file_count\":%lu}",
        (unsigned long)cluster_size,
        (unsigned long)log_file_size,
        (unsigned long)log_file_count
    );

    if (length < 0 || (size_t)length >= sizeof(content))
    {
        (void)FatFS_SD_CloseFile(&File);
        return FR_INVALID_PARAMETER;
    }

    res = FatFS_SD_WriteFile(&File, content, (uint32_t)length);

    (void)FatFS_SD_CloseFile(&File);

    return res;
}

static void appHandleFormattingRequest(void)
{
    FRESULT res;
    _Bool ReformattingRequested;
    FatFsDeviceType DevTmp;
    const char FormatRequestFileName[] =
        FILEHANDLER_FORMATTING_REQUEST_FILENAME;
    const char LogConfigFileName[] = "log_config.json";
    uint32_t cluster_size          = CLUSTER_SIZE;
    uint32_t log_file_size         = MAX_LOG_FILE_SIZE;
    uint32_t log_file_count        = MAX_LOG_FILE_COUNT;
    bool log_config_loaded         = false;

    log_config_loaded = appLoadLogConfigFile(
        LogConfigFileName,
        &cluster_size,
        &log_file_size,
        &log_file_count
    );

    res = FatFS_SD_OpenFileForRead(&DevTmp, FormatRequestFileName);

    if (FR_OK == res)
    {
        ReformattingRequested = true;

        do
        {
            uint32_t fileSize = 0U;
            char buffer[128];
            uint32_t readSize = 0U;

            res = FatFS_SD_GetFileSize(&DevTmp, &fileSize);
            if (FR_OK != res || fileSize == 0U)
            {
                break;
            }

            readSize = (fileSize < (sizeof(buffer) - 1U))
                           ? fileSize
                           : (sizeof(buffer) - 1U);
            res      = FatFS_SD_ReadFile(&DevTmp, buffer, readSize);
            if (FR_OK != res)
            {
                break;
            }

            buffer[readSize] = '\0';
            (void)appParseLogConfigJson(
                buffer,
                readSize,
                &cluster_size,
                &log_file_size,
                &log_file_count
            );
        } while (0);

        (void)FatFS_SD_CloseFile(&DevTmp);
    }
    else
    {
        ReformattingRequested = false;
    }

    appCanLogSetFileConfig(log_file_size, log_file_count);
    appCanLogSetClusterSize(cluster_size);

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

        if (FR_OK == res)
        {
            (void)appStoreLogConfigFile(
                LogConfigFileName,
                cluster_size,
                log_file_size,
                log_file_count
            );
        }
    }
    else if (!log_config_loaded)
    {
        (void)appStoreLogConfigFile(
            LogConfigFileName,
            cluster_size,
            log_file_size,
            log_file_count
        );
    }
}

static void appCanCtrlDataSetter(
    CanCtrlDataType *data,
    const AppControlDataType *appData,
    const AppConfigType *appConfig
)
{
    data->sendingActive = appData->runCanTracer;
    data->can1.baudrate = appConfig->can1.baudrate;
    data->can1.mode     = appConfig->can1.mode;
    data->can2.baudrate = appConfig->can2.baudrate;
    data->can2.mode     = appConfig->can2.mode;
}

static void appConfigSetDefaults(AppConfigType *config)
{
    config->can1.baudrate = FDCAN_BAUDRATE_250000;
    config->can1.mode     = FDCAN_MODE_2;
    config->can2.baudrate = FDCAN_BAUDRATE_250000;
    config->can2.mode     = FDCAN_MODE_2;
}

static void Core0Task0Main(void *parameters)
{
    IapErrorType IapRes;
    static UBaseType_t MinUnusedStack;

    /* Unused parameters. */
    (void)parameters;
    (void)MinUnusedStack;

    // Mmc_Init();

    /* Initialize HAL SysTick external timer */
    if (0 != TIM_HAL_Init(TIM_HAL_TIME_FREQ))
    {
        Error_Handler();
    }

    /* initialialize port early to allow for taskless SPI communication */
    SpiTask_PortInit();

    IapRes = Iap_Init();

    if (IAP_E_OK != IapRes)
    {
        (void)Iap_DeInit();

        Error_Handler();
    }

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
        &AppCtrlData.commitLog
    );
    appCanLogHandlerInit(AppCtrlData.Log);
    Core0Task1_SetCanLogHandle(AppCtrlData.Log);

    GPIO_Mco1_Init();

    Instrumentation_Init();

    appConfigSetDefaults(&AppConfig);

    appCanCtrlDataSetter(
        &CanCtrlData,
        (const AppControlDataType *)&AppCtrlData,
        (const AppConfigType *)&AppConfig
    );
    appFdcanInit(&CanCtrlData);

    ConfigManager_Init(&AppCtrlData.Config, "CONF.TXT", &AppCtrlData.mountRes);
    if (ConfigManager_Initialize(&AppCtrlData.Config) == CONFIG_OK)
    {
        if (ConfigManager_LoadConfig(&AppCtrlData.Config, &AppConfig)
            != CONFIG_OK)
        {
            appConfigSetDefaults(&AppConfig);
        }
        else
        {
            appCanCtrlDataSetter(
                &CanCtrlData,
                (const AppControlDataType *)&AppCtrlData,
                (const AppConfigType *)&AppConfig
            );
            appCanCtrlSetBaudrate(&CanCtrlData);
            appCanCtrlSetMode(&CanCtrlData);

            appCanLogSetParam(
                CLM_PARAMETER_ID_CAN1_BAUDRATE,
                AppConfig.can1.baudrate
            );
            appCanLogSetParam(
                CLM_PARAMETER_ID_CAN2_BAUDRATE,
                AppConfig.can2.baudrate
            );
        }

        SettingsHandler_Init(&AppConfig);
    }

    /* Release CanSendTask */
    CanSendTask_Notify();

    run = 1U;

    while (run)
    {
        http_poll();
        Iap_VerifyPoll();

        if (SettingsHandler_Poll(&AppConfig))
        {
            if (ConfigManager_UpdateConfig(
                    &AppCtrlData.Config,
                    &AppConfig,
                    true
                )
                == CONFIG_OK)
            {
                appCanLogSetParam(
                    CLM_PARAMETER_ID_CAN1_BAUDRATE,
                    AppConfig.can1.baudrate
                );
                appCanLogSetParam(
                    CLM_PARAMETER_ID_CAN2_BAUDRATE,
                    AppConfig.can2.baudrate
                );
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
                (const AppConfigType *)&AppConfig
            );
            appCanCtrlSetBaudrate(&CanCtrlData);
            appCanCtrlSetMode(&CanCtrlData);
            AppCtrlData.applyConfig = 0;
        }
        else
        {
            AppCtrlData.applyConfig = 0;
        }

        if (0 != AppCtrlData.applyFirmwareUpdate)
        {
            if (0 == AppCtrlData.runCanTracer && 0u != Iap_IsVerified())
            {
                FwUpdateHandoffStatusType handoff_status =
                    FW_UPDATE_HANDOFF_E_OK;

                if (RES_OK == AppCtrlData.mountRes)
                {
                    FatFS_SD_Unmount();
                    AppCtrlData.mountRes = RES_NOTRDY;
                }

                AppCtrlData.applyFirmwareUpdate = 0;
                handoff_status = FwUpdateHandoff_RequestApply();
                if (handoff_status == FW_UPDATE_HANDOFF_E_OK)
                {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    NVIC_SystemReset();
                }
            }
            else
            {
                AppCtrlData.applyFirmwareUpdate = 0;
            }
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

    TASK_CREATE_STATIC(
        CORE0_TASK2_FUNCTION,
        CORE0_TASK2_STACK_SIZE,
        CORE0_TASK2_PRIO
    );
}

/* Hooks */

int ConfigManager_SerializeHook(
    const void *config,
    char *buffer,
    uint32_t maxLength,
    uint32_t *length
)
{
    if (!config || !buffer || !length)
    {
        return -1;
    }

    return SettingsHandler_CreateJsonString(
        (AppConfigType *)config,
        buffer,
        maxLength,
        length
    );
}

int ConfigManager_DeserializeHook(
    const char *buffer,
    uint32_t length,
    void *config
)
{
    if (!buffer || !config)
    {
        return -1;
    }

    if (SETTINGS_OK
        == SettingsHandler_ParseConfig(
            (char *)buffer,
            length,
            (AppConfigType *)config
        ))
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

void WebInterface_ResetFirmwareVerifyHook(void)
{
    Iap_VerifyReset();
}

void WebInterface_RequestFirmwareVerifyHook(void)
{
    Iap_VerifyRequest();
}

void WebInterface_GetFirmwareVerifyStatusHook(
    uint8_t *state,
    uint32_t *processed,
    uint32_t *total
)
{
    Iap_GetVerifyStatus((IapVerifyStateType *)state, processed, total);
}

uint8_t WebInterface_IsFirmwareVerifiedHook(void)
{
    return Iap_IsVerified();
}

uint8_t WebInterface_PrepareFirmwareUploadHook(void)
{
    return (Iap_PrepareUploadSlot() == IAP_PREPARE_E_OK) ? 1u : 0u;
}

void WebInterface_RequestFirmwareApplyHook(void)
{
    AppCtrlData.applyFirmwareUpdate = 1;
}

uint8_t FwUpdateHandoff_GetMarkerAddress_Hook(uint32_t *address)
{
    if (NULL == address)
    {
        return FW_UPDATE_HANDOFF_E_PARAM;
    }

    *address = (uint32_t)(uintptr_t)&__scratch_end__;

    return FW_UPDATE_HANDOFF_E_OK;
}
