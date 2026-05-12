#ifndef MOTION_SPEED_H
#define MOTION_SPEED_H

#include <stdint.h>

#include "ai_error.h"

#define MOTION_SPEED_WHEEL_COUNT (4U)

typedef struct
{
    int32_t wheel1;
    int32_t wheel2;
    int32_t wheel3;
    int32_t wheel4;
} motion_speed_encoder_total_t;

typedef struct
{
    int32_t wheel1;
    int32_t wheel2;
    int32_t wheel3;
    int32_t wheel4;
} motion_speed_encoder_delta_t;

typedef struct
{
    float wheel1;
    float wheel2;
    float wheel3;
    float wheel4;
} motion_speed_wheel_float_t;

typedef struct
{
    float kp;
    float ki;
    float kd;
    float duty_limit_percent;
    float max_speed_mm_s;
    float feedforward_duty_per_mm_s;
    float static_duty_percent;
    float static_threshold_mm_s;
    float wheel_diameter_mm;
    int32_t counts_per_rev_x100[MOTION_SPEED_WHEEL_COUNT];
} motion_speed_config_t;

typedef struct
{
    motion_speed_wheel_float_t target_mm_s;
    motion_speed_wheel_float_t measured_mm_s;
    motion_speed_wheel_float_t duty_percent;
    motion_speed_encoder_delta_t encoder_delta;
    uint32_t dt_ms;
} motion_speed_sample_t;

typedef struct
{
    motion_speed_config_t config;
    motion_speed_encoder_total_t last_total;
    uint32_t last_ms;
    uint8_t has_last_sample;
    motion_speed_wheel_float_t target_mm_s;
    motion_speed_wheel_float_t measured_mm_s;
    motion_speed_wheel_float_t duty_percent;
    float integral_duty[MOTION_SPEED_WHEEL_COUNT];
} motion_speed_controller_t;

void motion_speed_default_config(motion_speed_config_t *config);
ai_status_t motion_speed_init(motion_speed_controller_t *controller,
                              const motion_speed_config_t *config);
ai_status_t motion_speed_set_config(motion_speed_controller_t *controller,
                                    const motion_speed_config_t *config);
ai_status_t motion_speed_set_targets(motion_speed_controller_t *controller,
                                     float wheel1_mm_s,
                                     float wheel2_mm_s,
                                     float wheel3_mm_s,
                                     float wheel4_mm_s);
void motion_speed_stop(motion_speed_controller_t *controller);
ai_status_t motion_speed_update(motion_speed_controller_t *controller,
                                const motion_speed_encoder_total_t *total,
                                uint32_t now_ms,
                                motion_speed_sample_t *sample);
float motion_speed_get_integral_duty(const motion_speed_controller_t *controller,
                                     uint8_t wheel_index);

#endif
