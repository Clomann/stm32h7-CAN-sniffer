#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "CanSendTask.h"
#include "Core0TasksCfg.h"

#include "CanCtrl.h"

static _Bool SendingActive = 0;

TASK_VARIABLES(CORE0_TASK1_FUNCTION, CORE0_TASK1_STACK_SIZE)

void CanSendTaskInit()
{
    TASK_CREATE_STATIC(
        CORE0_TASK1_FUNCTION,
        CORE0_TASK1_STACK_SIZE,
        CORE0_TASK1_PRIO
    );
}

void CanSendTask_Notify()
{
    xTaskNotifyGive(CanSendTaskHdl);
}

void CanSendTask_SetSendingActive(_Bool active)
{
    SendingActive = active;
}

void CanSendTask(void *arg)
{
    static TickType_t xPreviousWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    (void) (arg);
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    xPreviousWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xPreviousWakeTime, xFrequency);

        appFdcanPoll(SendingActive);
    }
}
