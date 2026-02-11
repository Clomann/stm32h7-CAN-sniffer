#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "SdBridgeTask.h"
#include "TasksHooks.h"
#include "Core0TasksCfg.h"
#include "FileHandlerTypes.h"

TASK_VARIABLES(CORE0_TASK5_FUNCTION, CORE0_TASK5_STACK_SIZE)

void SdBridgeTask_Init()
{

    TASK_CREATE_STATIC(
        CORE0_TASK5_FUNCTION,
        CORE0_TASK5_STACK_SIZE,
        CORE0_TASK5_PRIO
    );

    configASSERT(SdBridgeTaskHdl != NULL);
}

void SdBridgeTask(void *arg)
{
    static UBaseType_t MinUnusedStack;

    (void)MinUnusedStack;
    (void)(arg);

    ulTaskNotifyTake(pdTRUE, 0);

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        SdBridgeTask_ActionHook();

        MinUnusedStack = uxTaskGetStackHighWaterMark(NULL);

        if (MinUnusedStack < 50)
        {
            Tasks_ErrorHandler();
        }
    }
}

void SdBridgeTask_Notify()
{
    if (NULL != SdBridgeTaskHdl)
    {
        xTaskNotifyGive(SdBridgeTaskHdl);
    }
    else
    {
        Tasks_ErrorHandler();
    }
}
