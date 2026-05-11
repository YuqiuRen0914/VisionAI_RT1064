#include "event.h"

#include "ai_config.h"
#include "os_port.h"

#define EVENT_QUEUE_SIZE 16U

#if AI_USE_FREERTOS
static os_queue_t event_queue;
#else
static event_msg_t event_queue[EVENT_QUEUE_SIZE];
static uint8_t event_read_index;
static uint8_t event_write_index;
static uint8_t event_count;
#endif

void event_service_init(void)
{
#if AI_USE_FREERTOS
    (void)os_port_queue_create(&event_queue, EVENT_QUEUE_SIZE, sizeof(event_msg_t));
#else
    event_read_index = 0;
    event_write_index = 0;
    event_count = 0;
#endif
}

ai_status_t event_publish(const event_msg_t *msg)
{
    if(msg == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

#if AI_USE_FREERTOS
    if(event_queue == NULL)
    {
        return AI_ERR;
    }

    return os_port_queue_send(event_queue, msg, 0);
#else
    if(event_count >= EVENT_QUEUE_SIZE)
    {
        return AI_ERR_BUSY;
    }

    event_queue[event_write_index] = *msg;
    event_write_index = (uint8_t)((event_write_index + 1U) % EVENT_QUEUE_SIZE);
    event_count++;

    return AI_OK;
#endif
}

ai_status_t event_poll(event_msg_t *msg)
{
    if(msg == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

#if AI_USE_FREERTOS
    if(event_queue == NULL)
    {
        return AI_ERR;
    }

    return os_port_queue_receive(event_queue, msg, 0);
#else
    if(event_count == 0U)
    {
        return AI_ERR_NO_DATA;
    }

    *msg = event_queue[event_read_index];
    event_read_index = (uint8_t)((event_read_index + 1U) % EVENT_QUEUE_SIZE);
    event_count--;

    return AI_OK;
#endif
}
