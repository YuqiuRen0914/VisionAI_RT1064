#include "motion_action_runtime.h"

#include "ai_config.h"
#include "motion_defaults.h"
#include "zf_common_interrupt.h"

static motion_action_controller_t motion_action_runtime_controller;
static motion_action_config_t motion_action_runtime_config;
static motion_speed_controller_t motion_action_runtime_speed_controller;
static motion_speed_config_t motion_action_runtime_speed_config;
static motion_speed_wheel_float_t motion_action_runtime_speed_targets;
static motion_speed_sample_t motion_action_runtime_speed_sample;
static motion_action_result_t motion_action_runtime_pending_result;
static motion_action_debug_t motion_action_runtime_latest_debug;
static motion_action_runtime_adapter_t motion_action_runtime_adapter;

static void motion_action_runtime_zero_speed_targets(void)
{
    motion_action_runtime_speed_targets.wheel1 = 0.0f;
    motion_action_runtime_speed_targets.wheel2 = 0.0f;
    motion_action_runtime_speed_targets.wheel3 = 0.0f;
    motion_action_runtime_speed_targets.wheel4 = 0.0f;
}

static void motion_action_runtime_zero_result(motion_action_result_t *result)
{
    result->kind = MOTION_ACTION_RESULT_NONE;
    result->error = 0U;
    result->context = 0U;
    result->elapsed_ms = 0U;
}

static void motion_action_runtime_zero_speed_sample(motion_speed_sample_t *sample)
{
    sample->target_mm_s.wheel1 = 0.0f;
    sample->target_mm_s.wheel2 = 0.0f;
    sample->target_mm_s.wheel3 = 0.0f;
    sample->target_mm_s.wheel4 = 0.0f;
    sample->raw_measured_mm_s.wheel1 = 0.0f;
    sample->raw_measured_mm_s.wheel2 = 0.0f;
    sample->raw_measured_mm_s.wheel3 = 0.0f;
    sample->raw_measured_mm_s.wheel4 = 0.0f;
    sample->measured_mm_s.wheel1 = 0.0f;
    sample->measured_mm_s.wheel2 = 0.0f;
    sample->measured_mm_s.wheel3 = 0.0f;
    sample->measured_mm_s.wheel4 = 0.0f;
    sample->duty_percent.wheel1 = 0.0f;
    sample->duty_percent.wheel2 = 0.0f;
    sample->duty_percent.wheel3 = 0.0f;
    sample->duty_percent.wheel4 = 0.0f;
    sample->encoder_delta.wheel1 = 0;
    sample->encoder_delta.wheel2 = 0;
    sample->encoder_delta.wheel3 = 0;
    sample->encoder_delta.wheel4 = 0;
    sample->dt_ms = 0;
}

static void motion_action_runtime_zero_debug(motion_action_debug_t *debug)
{
    debug->cmd = 0;
    debug->dir = 0;
    debug->val = 0;
    debug->target_distance_mm = 0.0f;
    debug->x_mm = 0.0f;
    debug->y_mm = 0.0f;
    debug->axis_error_mm = 0.0f;
    debug->target_angle_deg = 0.0f;
    debug->heading_deg = 0.0f;
    debug->heading_error_deg = 0.0f;
    debug->imu_rate_dps = 0.0f;
    debug->vx_mm_s = 0.0f;
    debug->vy_mm_s = 0.0f;
    debug->rot_mm_s = 0.0f;
    debug->elapsed_ms = 0U;
    debug->completion_elapsed_ms = 0U;
    debug->blocked_ms = 0U;
}

static void motion_action_runtime_apply_zero_duty(void)
{
    motion_speed_wheel_float_t duty = {0.0f, 0.0f, 0.0f, 0.0f};

    if(motion_action_runtime_adapter.apply_duty != 0)
    {
        motion_action_runtime_adapter.apply_duty(&duty);
    }
}

static void motion_action_runtime_apply_sample_duty(const motion_speed_sample_t *sample)
{
    if(motion_action_runtime_adapter.apply_duty != 0)
    {
        motion_action_runtime_adapter.apply_duty(&sample->duty_percent);
    }
}

static void motion_action_runtime_stop_drive(void)
{
    motion_speed_stop(&motion_action_runtime_speed_controller);
    motion_action_runtime_zero_speed_targets();
    motion_action_runtime_apply_zero_duty();
}

static void motion_action_runtime_store_debug(const motion_action_debug_t *debug)
{
    uint32_t primask = interrupt_global_disable();

    motion_action_runtime_latest_debug = *debug;
    interrupt_global_enable(primask);
}

static void motion_action_runtime_clear_result(void)
{
    uint32_t primask = interrupt_global_disable();

    motion_action_runtime_zero_result(&motion_action_runtime_pending_result);
    interrupt_global_enable(primask);
}

static void motion_action_runtime_store_result(uint8_t kind,
                                               uint8_t error,
                                               uint8_t context,
                                               uint32_t elapsed_ms)
{
    uint32_t primask = interrupt_global_disable();

    motion_action_runtime_pending_result.kind = kind;
    motion_action_runtime_pending_result.error = error;
    motion_action_runtime_pending_result.context = context;
    motion_action_runtime_pending_result.elapsed_ms = elapsed_ms;
    interrupt_global_enable(primask);
}

static void motion_action_runtime_feedback_sensor_invalid(motion_action_feedback_t *feedback)
{
    motion_action_zero_feedback(feedback);
    feedback->dt_ms = AI_MOTION_PERIOD_MS;
    feedback->sensors_valid = 0U;
}

static void motion_action_runtime_fill_feedback(const motion_speed_sample_t *speed_sample,
                                                const motion_action_runtime_observation_t *observation,
                                                motion_action_feedback_t *feedback)
{
    motion_action_zero_feedback(feedback);
    feedback->encoder_delta = speed_sample->encoder_delta;
    feedback->measured_mm_s = speed_sample->measured_mm_s;
    feedback->imu_heading_deg = observation->imu_heading_deg;
    feedback->imu_rate_dps = observation->imu_rate_dps;
    feedback->dt_ms = speed_sample->dt_ms;
    feedback->sensors_valid = 1U;
}

static void motion_action_runtime_handle_output(const motion_action_output_t *output)
{
    motion_action_runtime_store_debug(&output->debug);

    if(output->state == MOTION_ACTION_STATE_DONE)
    {
        motion_action_runtime_store_result(MOTION_ACTION_RESULT_DONE, 0U, 0U, output->elapsed_ms);
        motion_action_stop(&motion_action_runtime_controller);
        motion_action_runtime_stop_drive();
    }
    else if(output->state == MOTION_ACTION_STATE_ERROR)
    {
        motion_action_runtime_store_result(MOTION_ACTION_RESULT_ERROR,
                                           output->error,
                                           output->context,
                                           output->elapsed_ms);
        motion_action_stop(&motion_action_runtime_controller);
        motion_action_runtime_stop_drive();
    }
    else if(output->state == MOTION_ACTION_STATE_ACTIVE)
    {
        motion_action_runtime_speed_targets = output->wheel_targets;
        (void)motion_speed_set_targets(&motion_action_runtime_speed_controller,
                                       motion_action_runtime_speed_targets.wheel1,
                                       motion_action_runtime_speed_targets.wheel2,
                                       motion_action_runtime_speed_targets.wheel3,
                                       motion_action_runtime_speed_targets.wheel4);
    }
}

static ai_status_t motion_action_runtime_validate_adapter(const motion_action_runtime_adapter_t *adapter)
{
    if((adapter == 0) ||
       (adapter->read_observation == 0) ||
       (adapter->read_heading_deg == 0) ||
       (adapter->now_ms == 0) ||
       (adapter->apply_duty == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static uint8_t motion_action_runtime_tuning_is_positive(float value)
{
    return (value > 0.0f) ? 1U : 0U;
}

static ai_status_t motion_action_runtime_reinit_idle_controller(void)
{
    motion_action_runtime_zero_debug(&motion_action_runtime_latest_debug);
    return motion_action_init(&motion_action_runtime_controller, &motion_action_runtime_config);
}

static void motion_action_runtime_config_to_tuning(const motion_action_config_t *config,
                                                   motion_action_tuning_t *tuning)
{
    tuning->move.max_speed_mm_s = config->move_max_speed_mm_s;
    tuning->move.accel_mm_s2 = config->move_accel_mm_s2;
    tuning->move.kp_mm_s_per_mm = config->move_kp_mm_s_per_mm;
    tuning->move.approach_speed_mm_s = config->move_approach_speed_mm_s;
    tuning->rotate.max_speed_mm_s = config->rotate_max_speed_mm_s;
    tuning->rotate.accel_mm_s2 = config->rotate_accel_mm_s2;
    tuning->rotate.kp_mm_s_per_deg = config->rotate_kp_mm_s_per_deg;
    tuning->rotate.approach_speed_mm_s = config->rotate_approach_speed_mm_s;
    tuning->heading.kp_mm_s_per_deg = config->heading_hold_kp_mm_s_per_deg;
    tuning->heading.max_rot_mm_s = config->heading_hold_max_rot_mm_s;
}

ai_status_t motion_action_runtime_init(const motion_action_runtime_adapter_t *adapter)
{
    if(motion_action_runtime_validate_adapter(adapter) != AI_OK)
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_runtime_adapter = *adapter;
    motion_defaults_load_speed_config(&motion_action_runtime_speed_config);
    motion_defaults_load_action_config(&motion_action_runtime_config);
    motion_action_runtime_zero_speed_targets();
    motion_action_runtime_zero_speed_sample(&motion_action_runtime_speed_sample);
    motion_action_runtime_zero_result(&motion_action_runtime_pending_result);
    motion_action_runtime_zero_debug(&motion_action_runtime_latest_debug);

    if(motion_speed_init(&motion_action_runtime_speed_controller, &motion_action_runtime_speed_config) != AI_OK)
    {
        return AI_ERR;
    }

    if(motion_action_init(&motion_action_runtime_controller, &motion_action_runtime_config) != AI_OK)
    {
        return AI_ERR;
    }

    return AI_OK;
}

void motion_action_runtime_tick(void)
{
    motion_action_runtime_observation_t observation;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    ai_status_t action_status;
    ai_status_t speed_status;
    uint32_t now_ms;

    if(motion_action_is_active(&motion_action_runtime_controller) == 0U)
    {
        return;
    }

    if(motion_action_runtime_adapter.read_observation(&observation) != AI_OK)
    {
        motion_action_runtime_feedback_sensor_invalid(&feedback);
        action_status = motion_action_update(&motion_action_runtime_controller, &feedback, &output);
        if(action_status == AI_OK)
        {
            motion_action_runtime_handle_output(&output);
        }
        return;
    }

    now_ms = motion_action_runtime_adapter.now_ms();
    speed_status = motion_speed_update(&motion_action_runtime_speed_controller,
                                       &observation.encoder_total,
                                       now_ms,
                                       &motion_action_runtime_speed_sample);

    if(speed_status == AI_ERR_NO_DATA)
    {
        return;
    }

    if(speed_status != AI_OK)
    {
        motion_action_runtime_feedback_sensor_invalid(&feedback);
        action_status = motion_action_update(&motion_action_runtime_controller, &feedback, &output);
        if(action_status == AI_OK)
        {
            motion_action_runtime_handle_output(&output);
        }
        return;
    }

    motion_action_runtime_fill_feedback(&motion_action_runtime_speed_sample, &observation, &feedback);
    action_status = motion_action_update(&motion_action_runtime_controller, &feedback, &output);
    if(action_status != AI_OK)
    {
        return;
    }

    motion_action_runtime_handle_output(&output);

    if(output.state == MOTION_ACTION_STATE_ACTIVE)
    {
        motion_action_runtime_apply_sample_duty(&motion_action_runtime_speed_sample);
    }
}

ai_status_t motion_action_runtime_begin(uint8_t cmd, uint8_t dir, uint8_t val)
{
    motion_action_command_t command;
    motion_action_debug_t debug;
    float start_heading_deg;

    if(motion_action_is_active(&motion_action_runtime_controller) != 0U)
    {
        return AI_ERR_BUSY;
    }

    if(motion_action_runtime_adapter.read_heading_deg(&start_heading_deg) != AI_OK)
    {
        return AI_ERR_NO_DATA;
    }

    motion_action_runtime_stop_drive();
    motion_action_runtime_clear_result();
    motion_action_runtime_zero_speed_sample(&motion_action_runtime_speed_sample);

    if(motion_speed_init(&motion_action_runtime_speed_controller, &motion_action_runtime_speed_config) != AI_OK)
    {
        return AI_ERR;
    }

    command.cmd = cmd;
    command.dir = dir;
    command.val = val;

    if(motion_action_start(&motion_action_runtime_controller, &command, start_heading_deg) != AI_OK)
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_get_debug(&motion_action_runtime_controller, &debug);
    motion_action_runtime_store_debug(&debug);

    return AI_OK;
}

void motion_action_runtime_stop_all(void)
{
    motion_action_stop(&motion_action_runtime_controller);
    motion_action_runtime_clear_result();
    motion_action_runtime_zero_debug(&motion_action_runtime_latest_debug);
    motion_action_runtime_stop_drive();
}

uint8_t motion_action_runtime_take_result(motion_action_result_t *result)
{
    uint32_t primask;

    if(result == 0)
    {
        return 0U;
    }

    primask = interrupt_global_disable();

    if(motion_action_runtime_pending_result.kind == MOTION_ACTION_RESULT_NONE)
    {
        motion_action_runtime_zero_result(result);
        interrupt_global_enable(primask);
        return 0U;
    }

    *result = motion_action_runtime_pending_result;
    motion_action_runtime_zero_result(&motion_action_runtime_pending_result);
    interrupt_global_enable(primask);

    return 1U;
}

uint8_t motion_action_runtime_is_active(void)
{
    return motion_action_is_active(&motion_action_runtime_controller);
}

void motion_action_runtime_get_debug(motion_action_debug_t *debug)
{
    uint32_t primask;

    if(debug == 0)
    {
        return;
    }

    primask = interrupt_global_disable();
    *debug = motion_action_runtime_latest_debug;
    interrupt_global_enable(primask);
}

void motion_action_runtime_get_speed_sample(motion_speed_sample_t *sample)
{
    if(sample == 0)
    {
        return;
    }

    *sample = motion_action_runtime_speed_sample;
}

void motion_action_runtime_get_tuning(motion_action_tuning_t *tuning)
{
    uint32_t primask;

    if(tuning == 0)
    {
        return;
    }

    primask = interrupt_global_disable();
    motion_action_runtime_config_to_tuning(&motion_action_runtime_config, tuning);
    interrupt_global_enable(primask);
}

ai_status_t motion_action_runtime_set_move_tuning(const motion_action_move_tuning_t *tuning)
{
    if(tuning == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_action_runtime_is_active() != 0U)
    {
        return AI_ERR_BUSY;
    }

    if((motion_action_runtime_tuning_is_positive(tuning->max_speed_mm_s) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->accel_mm_s2) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->kp_mm_s_per_mm) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->approach_speed_mm_s) == 0U))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_runtime_config.move_max_speed_mm_s = tuning->max_speed_mm_s;
    motion_action_runtime_config.move_accel_mm_s2 = tuning->accel_mm_s2;
    motion_action_runtime_config.move_kp_mm_s_per_mm = tuning->kp_mm_s_per_mm;
    motion_action_runtime_config.move_approach_speed_mm_s = tuning->approach_speed_mm_s;

    return motion_action_runtime_reinit_idle_controller();
}

ai_status_t motion_action_runtime_set_rotate_tuning(const motion_action_rotate_tuning_t *tuning)
{
    if(tuning == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_action_runtime_is_active() != 0U)
    {
        return AI_ERR_BUSY;
    }

    if((motion_action_runtime_tuning_is_positive(tuning->max_speed_mm_s) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->accel_mm_s2) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->kp_mm_s_per_deg) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->approach_speed_mm_s) == 0U))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_runtime_config.rotate_max_speed_mm_s = tuning->max_speed_mm_s;
    motion_action_runtime_config.rotate_accel_mm_s2 = tuning->accel_mm_s2;
    motion_action_runtime_config.rotate_kp_mm_s_per_deg = tuning->kp_mm_s_per_deg;
    motion_action_runtime_config.rotate_approach_speed_mm_s = tuning->approach_speed_mm_s;

    return motion_action_runtime_reinit_idle_controller();
}

ai_status_t motion_action_runtime_set_heading_tuning(const motion_action_heading_tuning_t *tuning)
{
    if(tuning == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_action_runtime_is_active() != 0U)
    {
        return AI_ERR_BUSY;
    }

    if((motion_action_runtime_tuning_is_positive(tuning->kp_mm_s_per_deg) == 0U) ||
       (motion_action_runtime_tuning_is_positive(tuning->max_rot_mm_s) == 0U))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_runtime_config.heading_hold_kp_mm_s_per_deg = tuning->kp_mm_s_per_deg;
    motion_action_runtime_config.heading_hold_max_rot_mm_s = tuning->max_rot_mm_s;

    return motion_action_runtime_reinit_idle_controller();
}

ai_status_t motion_action_runtime_restore_default_tuning(void)
{
    motion_action_config_t defaults;

    if(motion_action_runtime_is_active() != 0U)
    {
        return AI_ERR_BUSY;
    }

    motion_defaults_load_action_config(&defaults);
    motion_action_runtime_config.move_max_speed_mm_s = defaults.move_max_speed_mm_s;
    motion_action_runtime_config.move_accel_mm_s2 = defaults.move_accel_mm_s2;
    motion_action_runtime_config.move_kp_mm_s_per_mm = defaults.move_kp_mm_s_per_mm;
    motion_action_runtime_config.move_approach_speed_mm_s = defaults.move_approach_speed_mm_s;
    motion_action_runtime_config.rotate_max_speed_mm_s = defaults.rotate_max_speed_mm_s;
    motion_action_runtime_config.rotate_accel_mm_s2 = defaults.rotate_accel_mm_s2;
    motion_action_runtime_config.rotate_kp_mm_s_per_deg = defaults.rotate_kp_mm_s_per_deg;
    motion_action_runtime_config.rotate_approach_speed_mm_s = defaults.rotate_approach_speed_mm_s;
    motion_action_runtime_config.heading_hold_kp_mm_s_per_deg = defaults.heading_hold_kp_mm_s_per_deg;
    motion_action_runtime_config.heading_hold_max_rot_mm_s = defaults.heading_hold_max_rot_mm_s;

    return motion_action_runtime_reinit_idle_controller();
}
