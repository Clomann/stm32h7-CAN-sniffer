#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "SpiTask.h"
#include "SpiAbs.h"
#include "spi_port_freertos.h"

TASK_VARIABLES(CORE0_TASK4_FUNCTION, CORE0_TASK4_STACK_SIZE)

void SpiTask_Init()
{
    TASK_CREATE_STATIC(
        CORE0_TASK4_FUNCTION,
        CORE0_TASK4_STACK_SIZE,
        CORE0_TASK4_PRIO
    );
}

void SpiTask_PortInit(TaskHandle_t *handle)
{
    HAL_StatusTypeDef HalStatus;

    HalStatus = spi_port_freertos_init((TaskHandle_t *)&SpiTaskHdl);

    if (HalStatus != HAL_OK)
    {
        /* Initialization Error */
        SpiTask_ErrorHandlerHook();
    }

    SpiAbs_PwrOn(SPIABS_DEVICE_1);
}

void SpiAbs_PortDeInit()
{
    SpiAbs_PwrOff(SPIABS_DEVICE_1);
}

void SpiTask(void *parameters)
{
    SpiAbs_Task(parameters);
}

void SpiAbs_TaskControlCallback(uint32_t timeout)
{
    ulTaskNotifyTake(pdTRUE, timeout);
}

void SpiAbs_TaskSendReceiveCallback()
{
    if (SpiTaskHdl != NULL)
    {
        xTaskNotifyGive(SpiTaskHdl);
    }
    else
    {
        SpiTask_ErrorHandlerHook();
    }
}
