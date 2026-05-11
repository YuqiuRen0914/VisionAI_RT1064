#ifndef EVENT_H
#define EVENT_H

#include "ai_error.h"

typedef enum
{
    EVENT_ID_NONE = 0,
    EVENT_ID_VISION_RESULT,
    EVENT_ID_MOTION_COMMAND,
    EVENT_ID_COMM_RX,
    EVENT_ID_HEARTBEAT,
} event_id_t;

typedef struct
{
    event_id_t id;
    int32_t value0;
    int32_t value1;
} event_msg_t;

void event_service_init(void);
ai_status_t event_publish(const event_msg_t *msg);
ai_status_t event_poll(event_msg_t *msg);

#endif
