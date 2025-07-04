#include "Core0Task1.h"
#include "Core0TasksCfg.h"
#include "gpio.h"

static StaticTask_t Core0Task1MainTCB;
static StackType_t Core0Task1MainStack[ configMINIMAL_STACK_SIZE ];

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
    ( void ) xTaskCreateStatic( Core0Task1Main,
                                "Core0Task1Main",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                CORE0_TASK2_PRIO,
                                &( Core0Task1MainStack[ 0 ] ),
                                &( Core0Task1MainTCB ) );
}