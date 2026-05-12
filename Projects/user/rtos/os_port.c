#include "os_port.h"

#if AI_USE_FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#else
#include "zf_driver_delay.h"
#endif

ai_status_t os_port_create_task(os_task_entry_t entry,
                                const char *name,
                                uint16_t stack_words,
                                void *argument,
                                uint8_t priority)
{
#if AI_USE_FREERTOS
    BaseType_t created;

    created = xTaskCreate(entry, name, stack_words, argument, priority, NULL);
    return (created == pdPASS) ? AI_OK : AI_ERR;
#else
    (void)entry;
    (void)name;
    (void)stack_words;
    (void)argument;
    (void)priority;
    return AI_OK;
#endif
}

void os_port_start_scheduler(void)
{
#if AI_USE_FREERTOS
    vTaskStartScheduler();
#endif
}

void os_port_delay_ms(uint32_t ms)
{
#if AI_USE_FREERTOS
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    system_delay_ms(ms);
#endif
}

uint32_t os_port_now_ms(void)
{
#if AI_USE_FREERTOS
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return 0U;
#endif
}

ai_status_t os_port_queue_create(os_queue_t *queue, uint32_t item_count, uint32_t item_size)
{
    if(queue == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

#if AI_USE_FREERTOS
    *queue = (os_queue_t)xQueueCreate(item_count, item_size);
    return (*queue != NULL) ? AI_OK : AI_ERR;
#else
    (void)item_count;
    (void)item_size;
    *queue = NULL;
    return AI_ERR;
#endif
}

ai_status_t os_port_queue_send(os_queue_t queue, const void *item, uint32_t timeout_ms)
{
    if((queue == NULL) || (item == NULL))
    {
        return AI_ERR_INVALID_ARG;
    }

#if AI_USE_FREERTOS
    return (xQueueSend((QueueHandle_t)queue, item, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) ? AI_OK : AI_ERR_BUSY;
#else
    (void)timeout_ms;
    return AI_ERR;
#endif
}

ai_status_t os_port_queue_receive(os_queue_t queue, void *item, uint32_t timeout_ms)
{
    if((queue == NULL) || (item == NULL))
    {
        return AI_ERR_INVALID_ARG;
    }

#if AI_USE_FREERTOS
    return (xQueueReceive((QueueHandle_t)queue, item, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) ? AI_OK : AI_ERR_NO_DATA;
#else
    (void)timeout_ms;
    return AI_ERR_NO_DATA;
#endif
}
