#ifndef OS_PORT_H
#define OS_PORT_H

#include "ai_config.h"
#include "ai_error.h"

typedef void (*os_task_entry_t)(void *argument);
typedef void *os_queue_t;

ai_status_t os_port_create_task(os_task_entry_t entry,
                                const char *name,
                                uint16_t stack_words,
                                void *argument,
                                uint8_t priority);
void os_port_start_scheduler(void);
void os_port_delay_ms(uint32_t ms);
ai_status_t os_port_queue_create(os_queue_t *queue, uint32_t item_count, uint32_t item_size);
ai_status_t os_port_queue_send(os_queue_t queue, const void *item, uint32_t timeout_ms);
ai_status_t os_port_queue_receive(os_queue_t queue, void *item, uint32_t timeout_ms);

#endif
