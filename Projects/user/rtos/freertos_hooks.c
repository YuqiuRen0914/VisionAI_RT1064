#include "FreeRTOS.h"
#include "task.h"

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();

    while(1)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    taskDISABLE_INTERRUPTS();

    while(1)
    {
    }
}
