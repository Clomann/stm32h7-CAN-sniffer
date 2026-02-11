#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "CanBridgeTask.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"
#include "TasksHooks.h"
#include "RuntimeChecks.h"

TASK_VARIABLES(CORE0_TASK0_FUNCTION, CORE0_TASK0_STACK_SIZE)

void CanBridgeTaskInit()
{
    HAL_NVIC_SetPriority(
        DEFERRED_IRQn,
        DEFERRED_IRQ_PREEMPT_PRIO,
        0
    ); /* 6 ≥ configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */
    HAL_NVIC_EnableIRQ(DEFERRED_IRQn);

    TASK_CREATE_STATIC(
        CORE0_TASK0_FUNCTION,
        CORE0_TASK0_STACK_SIZE,
        CORE0_TASK0_PRIO
    );

    configASSERT(CanBridgeTaskHdl != NULL);
}

void CanBridgeTask(void *arg)
{
    FDCAN_ClassicFrameType Frame;
    static UBaseType_t MinUnusedStack;

    (void)MinUnusedStack;
    (void)(arg);

    ulTaskNotifyTake(pdTRUE, 0);

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        __asm volatile("nop");

        while (0 == CanAbs_Receive_Can1(&Frame))
        {
            fdcan_msg_port_receive(&Frame);
            CanBridgeTask_FrameCount++;
        }

        while (0 == CanAbs_Receive_Can2(&Frame))
        {
            fdcan_msg_port_receive(&Frame);
            CanBridgeTask_FrameCount++;
        }

        MinUnusedStack = uxTaskGetStackHighWaterMark(NULL);

        if (MinUnusedStack < 50)
        {
            Tasks_ErrorHandler();
        }
    }
}

void DEFERRED_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    HAL_NVIC_ClearPendingIRQ(DEFERRED_IRQn);

    /* “Give” one notification to the bridge task */
    vTaskNotifyGiveFromISR(
        CanBridgeTaskHdl,
        &xHigherPriorityTaskWoken
    ); /* may set it to pdTRUE */

    /* If the bridge task has a higher priority, switch to it
       immediately after exiting the ISR.  The macro name is
       port-specific: on Cortex-M it is usually portYIELD_FROM_ISR(). */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CanAbs_RxNotificationCallback()
{
    __DSB(); /* ensure writes complete */
    HAL_NVIC_SetPendingIRQ(DEFERRED_IRQn);
}
