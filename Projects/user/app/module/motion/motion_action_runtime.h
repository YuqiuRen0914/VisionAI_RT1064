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

typedef struct
{
    float max_speed_mm_s;
    float accel_mm_s2;
    float kp_mm_s_per_mm;
    float approach_speed_mm_s;
} motion_action_move_tuning_t;

typedef struct
{
    float max_speed_mm_s;
    float accel_mm_s2;
    float kp_mm_s_per_deg;
    float approach_speed_mm_s;
} motion_action_rotate_tuning_t;

typedef struct
{
    float kp_mm_s_per_deg;
    float max_rot_mm_s;
} motion_action_heading_tuning_t;

typedef struct
{
    motion_action_move_tuning_t move;
    motion_action_rotate_tuning_t rotate;
    motion_action_heading_tuning_t heading;
} motion_action_tuning_t;

typedef struct
{
    motion_speed_encoder_total_t encoder_total;
    float imu_heading_deg;
    float imu_rate_dps;
} motion_action_runtime_observation_t;

typedef ai_status_t (*motion_action_runtime_read_observation_t)(motion_action_runtime_observation_t *observation);
typedef ai_status_t (*motion_action_runtime_read_heading_t)(float *heading_deg);
typedef uint32_t (*motion_action_runtime_now_ms_t)(void);
typedef void (*motion_action_runtime_apply_duty_t)(const motion_speed_wheel_float_t *duty_percent);

typedef struct
{
    motion_action_runtime_read_observation_t read_observation;
    motion_action_runtime_read_heading_t read_heading_deg;
    motion_action_runtime_now_ms_t now_ms;
    motion_action_runtime_apply_duty_t apply_duty;
} motion_action_runtime_adapter_t;

ai_status_t motion_action_runtime_init(const motion_action_runtime_adapter_t *adapter);
void motion_action_runtime_tick(void);
ai_status_t motion_action_runtime_begin(uint8_t cmd, uint8_t dir, uint8_t val);
void motion_action_runtime_stop_all(void);
uint8_t motion_action_runtime_take_result(motion_action_result_t *result);
uint8_t motion_action_runtime_is_active(void);
void motion_action_runtime_get_debug(motion_action_debug_t *debug);
void motion_action_runtime_get_speed_sample(motion_speed_sample_t *sample);
void motion_action_runtime_get_tuning(motion_action_tuning_t *tuning);
ai_status_t motion_action_runtime_set_move_tuning(const motion_action_move_tuning_t *tuning);
ai_status_t motion_action_runtime_set_rotate_tuning(const motion_action_rotate_tuning_t *tuning);
ai_status_t motion_action_runtime_set_heading_tuning(const motion_action_heading_tuning_t *tuning);
ai_status_t motion_action_runtime_restore_default_tuning(void);

#endif
