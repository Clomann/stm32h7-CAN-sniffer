#pragma once

#include "FreeRTOSConfig.h"
#include "stm32h745xx.h"

#define DEFERRED_IRQn UART5_IRQn
#define DEFERRED_IRQHandler UART5_IRQHandler

#define CORE0_TASK0_FUNCTION CanBridgeTask
#define CORE0_TASK1_FUNCTION CanSendTask
#define CORE0_TASK2_FUNCTION Core0Task0Main
#define CORE0_TASK3_FUNCTION Core0Task1Main
#define CORE0_TASK4_FUNCTION SpiTask

#define CORE0_TASK0_STACK_SIZE (1U * configMINIMAL_STACK_SIZE)
#define CORE0_TASK1_STACK_SIZE (1U * configMINIMAL_STACK_SIZE)
#define CORE0_TASK2_STACK_SIZE (2U * configMINIMAL_STACK_SIZE)
#define CORE0_TASK3_STACK_SIZE (1U * configMINIMAL_STACK_SIZE)
#define CORE0_TASK4_STACK_SIZE (1U * configMINIMAL_STACK_SIZE)

#define CORE0_TASK0_PRIO (configMAX_PRIORITIES - 1U)
#define CORE0_TASK1_PRIO (configMAX_PRIORITIES - 2U)
#define CORE0_TASK2_PRIO (configMAX_PRIORITIES - 3U)
#define CORE0_TASK3_PRIO (configMAX_PRIORITIES - 3U)
#define CORE0_TASK4_PRIO (configMAX_PRIORITIES - 4U)


#define STRINGIFY(x) #x
#define FUNCTION_TO_STRING(func) STRINGIFY(func)

#define PASTE(a, b) a##b
#define TASK_VARIABLES(func, stack_size) \
    static volatile StaticTask_t PASTE(func,TCB); \
    static volatile TaskHandle_t PASTE(func,Hdl); \
    static volatile StackType_t PASTE(func,Stack[ stack_size ]);
#define TASK_CREATE_STATIC(func, stack_size, prio) \
    PASTE(func,Hdl) = xTaskCreateStatic( func, \
    FUNCTION_TO_STRING(func), \
    stack_size, \
    NULL, \
    prio, \
    (StackType_t*)&( PASTE(func,Stack)[ 0 ] ), \
    (StaticTask_t*)&( PASTE(func,TCB) ) );
