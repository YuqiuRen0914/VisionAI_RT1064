#ifndef MOTION_ACTION_H
#define MOTION_ACTION_H

#include <stdint.h>

#include "ai_error.h"
#include "motion_speed.h"

typedef enum
{
    MOTION_ACTION_STATE_IDLE = 0,
    MOTION_ACTION_STATE_ACTIVE,
    MOTION_ACTION_STATE_DONE,
    MOTION_ACTION_STATE_ERROR,
} motion_action_state_t;

typedef struct
{
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;
} motion_action_command_t;

typedef struct
{
    float wheel_diameter_mm;
    int32_t counts_per_rev_x100[MOTION_SPEED_WHEEL_COUNT];
    float move_max_speed_mm_s;
    float move_accel_mm_s2;
    float move_kp_mm_s_per_mm;
    float move_approach_speed_mm_s;
    float rotate_max_speed_mm_s;
    float rotate_accel_mm_s2;
    float rotate_kp_mm_s_per_deg;
    float rotate_approach_speed_mm_s;
    float heading_hold_kp_mm_s_per_deg;
    float heading_hold_max_rot_mm_s;
    float move_done_error_mm;
    float rotate_done_error_deg;
    float wheel_stop_mm_s;
    float imu_stop_dps;
    uint32_t completion_ms;
    uint32_t timeout_ms;
    uint32_t obstruction_ms;
    float obstruction_command_mm_s;
    int8_t odom_x_sign;
    int8_t odom_y_sign;
} motion_action_config_t;

typedef struct
{
    motion_speed_encoder_delta_t encoder_delta;
    motion_speed_wheel_float_t measured_mm_s;
    float imu_heading_deg;
    float imu_rate_dps;
    uint32_t dt_ms;
    uint8_t sensors_valid;
    uint8_t motor_fault;
    uint8_t control_unstable;
} motion_action_feedback_t;

typedef struct
{
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;
    float target_distance_mm;
    float x_mm;
    float y_mm;
    float axis_error_mm;
    float target_angle_deg;
    float heading_deg;
    float heading_error_deg;
    float imu_rate_dps;
    float vx_mm_s;
    float vy_mm_s;
    float rot_mm_s;
    uint32_t elapsed_ms;
    uint32_t completion_elapsed_ms;
    uint32_t blocked_ms;
} motion_action_debug_t;

typedef struct
{
    uint8_t state;
    uint8_t error;
    uint8_t context;
    uint32_t elapsed_ms;
    motion_speed_wheel_float_t wheel_targets;
    motion_action_debug_t debug;
} motion_action_output_t;

typedef struct
{
    motion_action_config_t config;
    uint8_t state;
    motion_action_command_t command;
    float heading_baseline_deg;
    float target_distance_mm;
    float target_angle_deg;
    float x_mm;
    float y_mm;
    float axis_speed_mm_s;
    float rot_mm_s;
    uint32_t elapsed_ms;
    uint32_t completion_elapsed_ms;
    uint32_t blocked_ms;
    motion_action_debug_t debug;
} motion_action_controller_t;

void motion_action_default_config(motion_action_config_t *config);
ai_status_t motion_action_init(motion_action_controller_t *controller,
                               const motion_action_config_t *config);
ai_status_t motion_action_start(motion_action_controller_t *controller,
                                const motion_action_command_t *command,
                                float start_heading_deg);
ai_status_t motion_action_update(motion_action_controller_t *controller,
                                 const motion_action_feedback_t *feedback,
                                 motion_action_output_t *output);
void motion_action_stop(motion_action_controller_t *controller);
void motion_action_zero_feedback(motion_action_feedback_t *feedback);
void motion_action_zero_output(motion_action_output_t *output);
void motion_action_get_debug(const motion_action_controller_t *controller,
                             motion_action_debug_t *debug);
uint8_t motion_action_is_active(const motion_action_controller_t *controller);

#endif
