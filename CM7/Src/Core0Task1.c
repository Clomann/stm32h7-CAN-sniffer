#include "Core0Task1.h"
#include "Core0TasksCfg.h"
#include "gpio.h"

TASK_VARIABLES(CORE0_TASK3_FUNCTION, CORE0_TASK3_STACK_SIZE)

static void Core0Task1Main( void * parameters )
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xCycleTime = pdMS_TO_TICKS(100);

    while (1)
    {
        GPIO_Dbg_Toggle();
        vTaskDelayUntil(&xLastWakeTime, xCycleTime);
    }
}

void Core0Task1Init()
{
    TASK_CREATE_STATIC(CORE0_TASK3_FUNCTION, CORE0_TASK3_STACK_SIZE, CORE0_TASK3_PRIO);
}