#include <FreeRTOS.h>
#include <task.h>

#include <string.h>

#include "profiling.h"
#include "CanAbs.h"

// In a header file or at the top of your file
#define MAX_TASKS 10

typedef struct {
    char name[16];
    uint64_t runtime_us;
    uint32_t percentage;
} task_runtime_info_t;

// Global variable you can inspect in GDB
volatile task_runtime_info_t g_task_stats[MAX_TASKS];
volatile uint32_t g_num_tasks = 0;
volatile uint64_t g_total_runtime_us = 0;

volatile TaskStatus_t task_array[MAX_TASKS];

__attribute__((used))
uint64_t Profiling_GetTimestamp(void)
{
    return FDCAN_GetTimestampHook();
}

// In main() or another function that runs
void force_profiling_link(void)
{
    // This forces the linker to keep the function
    volatile uint64_t (*func_ptr)(void) = &Profiling_GetTimestamp;
    (void)func_ptr;
}

void update_task_stats(void)
{
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    
    if (task_array != NULL && num_tasks <= MAX_TASKS) {
        uint32_t total_runtime;
        g_num_tasks = uxTaskGetSystemState(task_array, num_tasks, &total_runtime);
        
        g_total_runtime_us = total_runtime;
        
        for (UBaseType_t i = 0; i < num_tasks; i++) {
            strncpy(g_task_stats[i].name, task_array[i].pcTaskName, 15);
            g_task_stats[i].name[15] = '\0';
            g_task_stats[i].runtime_us = task_array[i].ulRunTimeCounter;
            g_task_stats[i].percentage = (task_array[i].ulRunTimeCounter * 100) / total_runtime;
        }
    }
}