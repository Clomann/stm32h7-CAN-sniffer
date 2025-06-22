#include "Core0Task0.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

static struct TaskParams {
    uint32_t TaskId;
};

static StaticTask_t exampleTaskTCB;
static StackType_t exampleTaskStack[ configMINIMAL_STACK_SIZE ];

static void exampleTask( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    for( ; ; )
    {
        /* Example Task Code */
        vTaskDelay( 100 ); /* delay 100 ticks */
    }
}

void Core0Task0Init()
{
    ( void ) xTaskCreateStatic( exampleTask,
                                "example",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( exampleTaskStack[ 0 ] ),
                                &( exampleTaskTCB ) );

    vTaskStartScheduler();
}