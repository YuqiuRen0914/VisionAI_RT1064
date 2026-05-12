#ifndef MOTION_ACTION_RUNTIME_H
#define MOTION_ACTION_RUNTIME_H

#include <stdint.h>

#include "ai_error.h"
#include "motion_action.h"
#include "motion_speed.h"

typedef enum
{
    MOTION_ACTION_RESULT_NONE = 0,
    MOTION_ACTION_RESULT_DONE,
    MOTION_ACTION_RESULT_ERROR,
} motion_action_result_kind_t;

typedef struct
{
    uint8_t kind;
    uint8_t error;
    uint8_t context;
    uint32_t elapsed_ms;
} motion_action_result_t;

typedef void (*motion_action_runtime_apply_duty_t)(int16_t wheel1,
                                                   int16_t wheel2,
                                                   int16_t wheel3,
                                                   int16_t wheel4);

ai_status_t motion_action_runtime_init(motion_action_runtime_apply_duty_t apply_duty);
void motion_action_runtime_tick(void);
ai_status_t motion_action_runtime_begin(uint8_t cmd, uint8_t dir, uint8_t val);
void motion_action_runtime_stop_all(void);
uint8_t motion_action_runtime_take_result(motion_action_result_t *result);
uint8_t motion_action_runtime_is_active(void);
void motion_action_runtime_get_debug(motion_action_debug_t *debug);
void motion_action_runtime_get_speed_sample(motion_speed_sample_t *sample);

#endif
