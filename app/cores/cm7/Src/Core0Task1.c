#include "Core0Task1.h"
#include "Core0TasksCfg.h"

#include "CanLogManager.h"
#include "FreeRTOS.h"
#include "task.h"

TASK_VARIABLES(CORE0_TASK3_FUNCTION, CORE0_TASK3_STACK_SIZE)

static CanLogControlDataType *CanLogHandle = NULL;

static void Core0Task1Main(void *parameters)
{
    (void)parameters;

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (1)
    {
        if (CanLogHandle != NULL)
        {
            appCanLogHandlerPoll(CanLogHandle);
        }
    }
}

void Core0Task1Init(void)
{
    TASK_CREATE_STATIC(
        CORE0_TASK3_FUNCTION,
        CORE0_TASK3_STACK_SIZE,
        CORE0_TASK3_PRIO
    );
}

void Core0Task1_SetCanLogHandle(CanLogControlDataType *handle)
{
    TaskHandle_t task_handle;

    CanLogHandle = handle;

    task_handle = (TaskHandle_t)Core0Task1MainHdl;
    if (task_handle != NULL)
    {
        xTaskNotifyGive(task_handle);
    }
}
