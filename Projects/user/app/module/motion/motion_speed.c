#include "motion_speed.h"

#define MOTION_SPEED_PI_F       (3.14159265358979323846f)
#define MOTION_SPEED_DEFAULT_KP (0.05f)
#define MOTION_SPEED_DEFAULT_KI (0.02f)

static float motion_speed_abs_f32(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float motion_speed_sign_f32(float value)
{
    if(value > 0.0f)
    {
        return 1.0f;
    }

    if(value < 0.0f)
    {
        return -1.0f;
    }

    return 0.0f;
}

static float motion_speed_clamp_f32(float value, float limit)
{
    if(limit < 0.0f)
    {
        limit = -limit;
    }

    if(value > limit)
    {
        return limit;
    }

    if(value < -limit)
    {
        return -limit;
    }

    return value;
}

static float motion_speed_get_wheel_float(const motion_speed_wheel_float_t *value, uint8_t index)
{
    if(index == 0U)
    {
        return value->wheel1;
    }

    if(index == 1U)
    {
        return value->wheel2;
    }

    if(index == 2U)
    {
        return value->wheel3;
    }

    return value->wheel4;
}

static void motion_speed_set_wheel_float(motion_speed_wheel_float_t *value,
                                         uint8_t index,
                                         float wheel_value)
{
    if(index == 0U)
    {
        value->wheel1 = wheel_value;
    }
    else if(index == 1U)
    {
        value->wheel2 = wheel_value;
    }
    else if(index == 2U)
    {
        value->wheel3 = wheel_value;
    }
    else
    {
        value->wheel4 = wheel_value;
    }
}

static int32_t motion_speed_get_total(const motion_speed_encoder_total_t *total, uint8_t index)
{
    if(index == 0U)
    {
        return total->wheel1;
    }

    if(index == 1U)
    {
        return total->wheel2;
    }

    if(index == 2U)
    {
        return total->wheel3;
    }

    return total->wheel4;
}

static void motion_speed_set_delta(motion_speed_encoder_delta_t *delta,
                                   uint8_t index,
                                   int32_t wheel_delta)
{
    if(index == 0U)
    {
        delta->wheel1 = wheel_delta;
    }
    else if(index == 1U)
    {
        delta->wheel2 = wheel_delta;
    }
    else if(index == 2U)
    {
        delta->wheel3 = wheel_delta;
    }
    else
    {
        delta->wheel4 = wheel_delta;
    }
}

static void motion_speed_zero_float_wheels(motion_speed_wheel_float_t *value)
{
    value->wheel1 = 0.0f;
    value->wheel2 = 0.0f;
    value->wheel3 = 0.0f;
    value->wheel4 = 0.0f;
}

static void motion_speed_zero_sample(motion_speed_sample_t *sample)
{
    motion_speed_zero_float_wheels(&sample->target_mm_s);
    motion_speed_zero_float_wheels(&sample->measured_mm_s);
    motion_speed_zero_float_wheels(&sample->duty_percent);
    sample->encoder_delta.wheel1 = 0;
    sample->encoder_delta.wheel2 = 0;
    sample->encoder_delta.wheel3 = 0;
    sample->encoder_delta.wheel4 = 0;
    sample->dt_ms = 0;
}

static ai_status_t motion_speed_validate_config(const motion_speed_config_t *config)
{
    if(config == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if((config->wheel_diameter_mm <= 0.0f) ||
       (config->duty_limit_percent <= 0.0f) ||
       (config->max_speed_mm_s <= 0.0f))
    {
        return AI_ERR_INVALID_ARG;
    }

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        if(config->counts_per_rev_x100[i] <= 0)
        {
            return AI_ERR_INVALID_ARG;
        }
    }

    return AI_OK;
}

static float motion_speed_counts_to_mm(const motion_speed_config_t *config,
                                       uint8_t wheel_index,
                                       int32_t counts)
{
    const float circumference_mm = config->wheel_diameter_mm * MOTION_SPEED_PI_F;

    return ((float)counts * circumference_mm * 100.0f) /
           (float)config->counts_per_rev_x100[wheel_index];
}

static float motion_speed_static_duty(const motion_speed_config_t *config, float target_mm_s)
{
    if((config->static_duty_percent <= 0.0f) ||
       (config->static_threshold_mm_s <= 0.0f) ||
       (target_mm_s == 0.0f) ||
       (motion_speed_abs_f32(target_mm_s) > config->static_threshold_mm_s))
    {
        return 0.0f;
    }

    return motion_speed_sign_f32(target_mm_s) * config->static_duty_percent;
}

static float motion_speed_update_wheel(motion_speed_controller_t *controller,
                                       uint8_t wheel_index,
                                       float target_mm_s,
                                       float measured_mm_s,
                                       float dt_s)
{
    const motion_speed_config_t *config = &controller->config;
    float error_mm_s;
    float proportional;
    float feedforward;
    float static_duty;
    float next_integral;
    float unsaturated;
    float saturated;

    if(target_mm_s == 0.0f)
    {
        controller->integral_duty[wheel_index] = 0.0f;
        return 0.0f;
    }

    error_mm_s = target_mm_s - measured_mm_s;
    proportional = config->kp * error_mm_s;
    next_integral = controller->integral_duty[wheel_index] + (config->ki * error_mm_s * dt_s);
    next_integral = motion_speed_clamp_f32(next_integral, config->duty_limit_percent);
    feedforward = config->feedforward_duty_per_mm_s * target_mm_s;
    static_duty = motion_speed_static_duty(config, target_mm_s);
    unsaturated = proportional + next_integral + feedforward + static_duty;
    saturated = motion_speed_clamp_f32(unsaturated, config->duty_limit_percent);

    if((unsaturated == saturated) ||
       ((unsaturated > saturated) && (error_mm_s < 0.0f)) ||
       ((unsaturated < saturated) && (error_mm_s > 0.0f)))
    {
        controller->integral_duty[wheel_index] = next_integral;
    }

    return saturated;
}

void motion_speed_default_config(motion_speed_config_t *config)
{
    if(config == 0)
    {
        return;
    }

    config->kp = MOTION_SPEED_DEFAULT_KP;
    config->ki = MOTION_SPEED_DEFAULT_KI;
    config->kd = 0.0f;
    config->duty_limit_percent = 30.0f;
    config->max_speed_mm_s = 200.0f;
    config->feedforward_duty_per_mm_s = 0.0f;
    config->static_duty_percent = 0.0f;
    config->static_threshold_mm_s = 30.0f;
    config->wheel_diameter_mm = 63.0f;

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        config->counts_per_rev_x100[i] = 239000;
    }
}

ai_status_t motion_speed_init(motion_speed_controller_t *controller,
                              const motion_speed_config_t *config)
{
    if((controller == 0) || (motion_speed_validate_config(config) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    controller->config = *config;
    controller->last_total.wheel1 = 0;
    controller->last_total.wheel2 = 0;
    controller->last_total.wheel3 = 0;
    controller->last_total.wheel4 = 0;
    controller->last_ms = 0;
    controller->has_last_sample = 0U;
    motion_speed_zero_float_wheels(&controller->target_mm_s);
    motion_speed_zero_float_wheels(&controller->measured_mm_s);
    motion_speed_zero_float_wheels(&controller->duty_percent);

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        controller->integral_duty[i] = 0.0f;
    }

    return AI_OK;
}

ai_status_t motion_speed_set_config(motion_speed_controller_t *controller,
                                    const motion_speed_config_t *config)
{
    if((controller == 0) || (motion_speed_validate_config(config) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    controller->config = *config;
    return AI_OK;
}

ai_status_t motion_speed_set_targets(motion_speed_controller_t *controller,
                                     float wheel1_mm_s,
                                     float wheel2_mm_s,
                                     float wheel3_mm_s,
                                     float wheel4_mm_s)
{
    if(controller == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    controller->target_mm_s.wheel1 = motion_speed_clamp_f32(wheel1_mm_s, controller->config.max_speed_mm_s);
    controller->target_mm_s.wheel2 = motion_speed_clamp_f32(wheel2_mm_s, controller->config.max_speed_mm_s);
    controller->target_mm_s.wheel3 = motion_speed_clamp_f32(wheel3_mm_s, controller->config.max_speed_mm_s);
    controller->target_mm_s.wheel4 = motion_speed_clamp_f32(wheel4_mm_s, controller->config.max_speed_mm_s);

    return AI_OK;
}

void motion_speed_stop(motion_speed_controller_t *controller)
{
    if(controller == 0)
    {
        return;
    }

    motion_speed_zero_float_wheels(&controller->target_mm_s);
    motion_speed_zero_float_wheels(&controller->duty_percent);

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        controller->integral_duty[i] = 0.0f;
    }
}

ai_status_t motion_speed_update(motion_speed_controller_t *controller,
                                const motion_speed_encoder_total_t *total,
                                uint32_t now_ms,
                                motion_speed_sample_t *sample)
{
    uint32_t dt_ms;
    float dt_s;

    if((controller == 0) || (total == 0) || (sample == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_zero_sample(sample);

    if(controller->has_last_sample == 0U)
    {
        controller->last_total = *total;
        controller->last_ms = now_ms;
        controller->has_last_sample = 1U;
        return AI_ERR_NO_DATA;
    }

    dt_ms = now_ms - controller->last_ms;
    if(dt_ms == 0U)
    {
        return AI_ERR_NO_DATA;
    }

    dt_s = (float)dt_ms / 1000.0f;
    sample->dt_ms = dt_ms;

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        const int32_t current_total = motion_speed_get_total(total, i);
        const int32_t last_total = motion_speed_get_total(&controller->last_total, i);
        const int32_t delta = current_total - last_total;
        const float measured_mm_s = motion_speed_counts_to_mm(&controller->config, i, delta) / dt_s;
        const float target_mm_s = motion_speed_get_wheel_float(&controller->target_mm_s, i);
        const float duty = motion_speed_update_wheel(controller, i, target_mm_s, measured_mm_s, dt_s);

        motion_speed_set_delta(&sample->encoder_delta, i, delta);
        motion_speed_set_wheel_float(&controller->measured_mm_s, i, measured_mm_s);
        motion_speed_set_wheel_float(&controller->duty_percent, i, duty);
        motion_speed_set_wheel_float(&sample->target_mm_s, i, target_mm_s);
        motion_speed_set_wheel_float(&sample->measured_mm_s, i, measured_mm_s);
        motion_speed_set_wheel_float(&sample->duty_percent, i, duty);
    }

    controller->last_total = *total;
    controller->last_ms = now_ms;
    return AI_OK;
}

float motion_speed_get_integral_duty(const motion_speed_controller_t *controller,
                                     uint8_t wheel_index)
{
    if((controller == 0) || (wheel_index >= MOTION_SPEED_WHEEL_COUNT))
    {
        return 0.0f;
    }

    return controller->integral_duty[wheel_index];
}
