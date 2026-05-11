#ifndef VISION_H
#define VISION_H

#include "ai_error.h"

typedef struct
{
    int16_t line_offset;
    int16_t target_speed;
    uint8_t valid;
} vision_result_t;

ai_status_t vision_module_init(void);
void vision_module_tick(void);

#endif
