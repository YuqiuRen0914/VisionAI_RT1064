#include "motion.h"

#include "ai_config.h"
#include "drive.h"
#include "drive_config.h"
#include "imu660ra.h"
#include "motion_defaults.h"
#include "os_port.h"
#include "zf_common_interrupt.h"

static motion_mode_t motion_mode;
static mecanum_duty_t motion_last_duty;

static mecanum_duty_t motion_test_duty;
static uint32_t motion_test_remaining_ms;
static volatile uint8_t motion_test_armed;

static motion_speed_controller_t motion_speed_controller;
static motion_speed_config_t motion_speed_config;
static motion_speed_wheel_float_t motion_speed_targets;
static motion_speed_sample_t motion_speed_last_sample;

static ai_status_t motion_drive_status_to_ai(DriveStatus status)
{
    return (status == DRIVE_STATUS_OK) ? AI_OK : AI_ERR;
}

static int16_t motion_abs_i16(int16_t value)
{
    if(value == INT16_MIN)
    {
        return INT16_MAX;
    }

    return (value < 0) ? (int16_t)-value : value;
}

static ai_status_t motion_validate_duty(int16_t duty_percent)
{
    if(duty_percent > AI_TEST_DUTY_PERCENT_MAX)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(duty_percent < -AI_TEST_DUTY_PERCENT_MAX)
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t motion_validate_run_ms(uint32_t run_ms)
{
    if(run_ms > AI_TEST_RUN_MS_MAX)
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static int16_t motion_float_to_duty(float duty_percent)
{
    int32_t rounded;

    if(duty_percent >= 0.0f)
    {
        rounded = (int32_t)(duty_percent + 0.5f);
    }
    else
    {
        rounded = (int32_t)(duty_percent - 0.5f);
    }

    if(rounded > DRIVE_DUTY_PERCENT_MAX)
    {
        return DRIVE_DUTY_PERCENT_MAX;
    }

    if(rounded < -DRIVE_DUTY_PERCENT_MAX)
    {
        return (int16_t)-DRIVE_DUTY_PERCENT_MAX;
    }

    return (int16_t)rounded;
}

static void motion_zero_duty(mecanum_duty_t *duty)
{
    duty->motor1 = 0;
    duty->motor2 = 0;
    duty->motor3 = 0;
    duty->motor4 = 0;
}

static void motion_zero_speed_targets(void)
{
    motion_speed_targets.wheel1 = 0.0f;
    motion_speed_targets.wheel2 = 0.0f;
    motion_speed_targets.wheel3 = 0.0f;
    motion_speed_targets.wheel4 = 0.0f;
}

static void motion_zero_speed_sample(motion_speed_sample_t *sample)
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

static void motion_apply_duty(int16_t wheel1, int16_t wheel2, int16_t wheel3, int16_t wheel4)
{
    motion_last_duty.motor1 = wheel1;
    motion_last_duty.motor2 = wheel2;
    motion_last_duty.motor3 = wheel3;
    motion_last_duty.motor4 = wheel4;
    (void)DriveSetAllDuty(wheel1, wheel2, wheel3, wheel4);
}

static void motion_stop_drive(void)
{
    motion_zero_duty(&motion_test_duty);
    motion_test_remaining_ms = 0;
    motion_speed_stop(&motion_speed_controller);
    motion_zero_speed_targets();
    motion_apply_duty(0, 0, 0, 0);
}

static void motion_load_speed_defaults(void)
{
    motion_defaults_load_speed_config(&motion_speed_config);
}

static void motion_drive_total_to_speed_total(const DriveEncoderTotal *drive_total,
                                              motion_speed_encoder_total_t *speed_total)
{
    speed_total->wheel1 = drive_total->wheel1;
    speed_total->wheel2 = drive_total->wheel2;
    speed_total->wheel3 = drive_total->wheel3;
    speed_total->wheel4 = drive_total->wheel4;
}

static void motion_test_apply_locked(const mecanum_duty_t *duty, uint32_t run_ms)
{
    uint32_t primask = interrupt_global_disable();

    motion_test_duty = *duty;
    motion_test_remaining_ms = run_ms;

    if(motion_test_remaining_ms == 0U)
    {
        motion_zero_duty(&motion_test_duty);
    }

    interrupt_global_enable(primask);
}

static void motion_test_tick(void)
{
    uint32_t primask;
    mecanum_duty_t duty;

    primask = interrupt_global_disable();
    if(motion_test_armed == 0U)
    {
        motion_test_remaining_ms = 0;
        motion_zero_duty(&motion_test_duty);
    }
    else if(motion_test_remaining_ms > AI_MOTION_PERIOD_MS)
    {
        motion_test_remaining_ms -= AI_MOTION_PERIOD_MS;
    }
    else if(motion_test_remaining_ms != 0U)
    {
        motion_test_remaining_ms = 0;
        motion_zero_duty(&motion_test_duty);
    }

    duty = motion_test_duty;
    interrupt_global_enable(primask);

    motion_apply_duty(duty.motor1, duty.motor2, duty.motor3, duty.motor4);
}

static void motion_speed_bench_tick(void)
{
    DriveEncoderTotal drive_total;
    motion_speed_encoder_total_t speed_total;
    uint32_t now_ms;

    if(DriveGetAllEncoderTotal(&drive_total) != DRIVE_STATUS_OK)
    {
        motion_apply_duty(0, 0, 0, 0);
        return;
    }

    motion_drive_total_to_speed_total(&drive_total, &speed_total);
    now_ms = os_port_now_ms();

    if(motion_speed_update(&motion_speed_controller,
                           &speed_total,
                           now_ms,
                           &motion_speed_last_sample) == AI_OK)
    {
        motion_apply_duty(motion_float_to_duty(motion_speed_last_sample.duty_percent.wheel1),
                          motion_float_to_duty(motion_speed_last_sample.duty_percent.wheel2),
                          motion_float_to_duty(motion_speed_last_sample.duty_percent.wheel3),
                          motion_float_to_duty(motion_speed_last_sample.duty_percent.wheel4));
    }
}

ai_status_t motion_module_init(void)
{
    ai_status_t status;

    motion_mode = MOTION_MODE_ACTION_CLOSED_LOOP;
    motion_zero_duty(&motion_last_duty);
    motion_zero_duty(&motion_test_duty);
    motion_test_remaining_ms = 0;
    motion_test_armed = 0;
    motion_load_speed_defaults();
    motion_zero_speed_targets();
    motion_zero_speed_sample(&motion_speed_last_sample);

    status = motion_drive_status_to_ai(DriveInit());
    if(status != AI_OK)
    {
        return status;
    }

    if(motion_speed_init(&motion_speed_controller, &motion_speed_config) != AI_OK)
    {
        return AI_ERR;
    }

    if(motion_action_runtime_init(motion_apply_duty) != AI_OK)
    {
        return AI_ERR;
    }

    return Imu660raInit();
}

void motion_module_tick(void)
{
    if(motion_mode == MOTION_MODE_OPEN_LOOP_TEST)
    {
        motion_test_tick();
    }
    else if(motion_mode == MOTION_MODE_SPEED_BENCH)
    {
        motion_speed_bench_tick();
    }
    else
    {
        motion_action_runtime_tick();
    }
}

motion_mode_t motion_get_mode(void)
{
    return motion_mode;
}

ai_status_t motion_test_arm(void)
{
    uint32_t primask = interrupt_global_disable();

    motion_mode = MOTION_MODE_OPEN_LOOP_TEST;
    motion_zero_duty(&motion_test_duty);
    motion_test_remaining_ms = 0;
    motion_test_armed = 1U;
    interrupt_global_enable(primask);

    motion_speed_stop(&motion_speed_controller);
    motion_zero_speed_targets();
    motion_apply_duty(0, 0, 0, 0);

    return AI_OK;
}

ai_status_t motion_test_disarm(void)
{
    return motion_test_stop();
}

uint8_t motion_test_is_armed(void)
{
    return ((motion_mode == MOTION_MODE_OPEN_LOOP_TEST) && (motion_test_armed != 0U)) ? 1U : 0U;
}

ai_status_t motion_test_stop(void)
{
    uint32_t primask = interrupt_global_disable();

    motion_zero_duty(&motion_test_duty);
    motion_test_remaining_ms = 0;
    motion_test_armed = 0;
    motion_mode = MOTION_MODE_ACTION_CLOSED_LOOP;
    interrupt_global_enable(primask);

    motion_stop_drive();

    return AI_OK;
}

ai_status_t motion_test_set_motor(uint8_t wheel_id, int16_t duty_percent, uint32_t run_ms)
{
    mecanum_duty_t duty = {0, 0, 0, 0};

    if((wheel_id < 1U) || (wheel_id > (uint8_t)DRIVE_WHEEL_COUNT))
    {
        return AI_ERR_INVALID_ARG;
    }

    if((motion_validate_duty(duty_percent) != AI_OK) ||
       (motion_validate_run_ms(run_ms) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_test_is_armed() == 0U)
    {
        return AI_ERR_BUSY;
    }

    if(wheel_id == 1U)
    {
        duty.motor1 = duty_percent;
    }
    else if(wheel_id == 2U)
    {
        duty.motor2 = duty_percent;
    }
    else if(wheel_id == 3U)
    {
        duty.motor3 = duty_percent;
    }
    else
    {
        duty.motor4 = duty_percent;
    }

    motion_test_apply_locked(&duty, run_ms);

    return AI_OK;
}

ai_status_t motion_test_set_all(int16_t wheel1_duty,
                                int16_t wheel2_duty,
                                int16_t wheel3_duty,
                                int16_t wheel4_duty,
                                uint32_t run_ms)
{
    mecanum_duty_t duty;

    if((motion_validate_duty(wheel1_duty) != AI_OK) ||
       (motion_validate_duty(wheel2_duty) != AI_OK) ||
       (motion_validate_duty(wheel3_duty) != AI_OK) ||
       (motion_validate_duty(wheel4_duty) != AI_OK) ||
       (motion_validate_run_ms(run_ms) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_test_is_armed() == 0U)
    {
        return AI_ERR_BUSY;
    }

    duty.motor1 = wheel1_duty;
    duty.motor2 = wheel2_duty;
    duty.motor3 = wheel3_duty;
    duty.motor4 = wheel4_duty;

    motion_test_apply_locked(&duty, run_ms);

    return AI_OK;
}

ai_status_t motion_test_set_mecanum(const mecanum_velocity_t *velocity, uint32_t run_ms)
{
    mecanum_duty_t duty;

    if(velocity == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_validate_run_ms(run_ms) != AI_OK)
    {
        return AI_ERR_INVALID_ARG;
    }

    if((motion_abs_i16(velocity->vx) > AI_TEST_DUTY_PERCENT_MAX) ||
       (motion_abs_i16(velocity->vy) > AI_TEST_DUTY_PERCENT_MAX) ||
       (motion_abs_i16(velocity->wz) > AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_test_is_armed() == 0U)
    {
        return AI_ERR_BUSY;
    }

    if(mecanum_solve_duty(velocity, &duty) != AI_OK)
    {
        return AI_ERR;
    }

    if(mecanum_normalize_duty(&duty, AI_TEST_DUTY_PERCENT_MAX) != AI_OK)
    {
        return AI_ERR;
    }

    if((motion_abs_i16(duty.motor1) > AI_TEST_DUTY_PERCENT_MAX) ||
       (motion_abs_i16(duty.motor2) > AI_TEST_DUTY_PERCENT_MAX) ||
       (motion_abs_i16(duty.motor3) > AI_TEST_DUTY_PERCENT_MAX) ||
       (motion_abs_i16(duty.motor4) > AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR;
    }

    motion_test_apply_locked(&duty, run_ms);

    return AI_OK;
}

void motion_test_get_duty(mecanum_duty_t *duty)
{
    uint32_t primask;

    if(duty == 0)
    {
        return;
    }

    primask = interrupt_global_disable();
    *duty = motion_last_duty;
    interrupt_global_enable(primask);
}

ai_status_t motion_speed_bench_arm(void)
{
    uint32_t primask;

    if(motion_speed_init(&motion_speed_controller, &motion_speed_config) != AI_OK)
    {
        return AI_ERR;
    }

    motion_zero_speed_targets();
    motion_zero_speed_sample(&motion_speed_last_sample);
    (void)motion_speed_set_targets(&motion_speed_controller, 0.0f, 0.0f, 0.0f, 0.0f);

    primask = interrupt_global_disable();
    motion_mode = MOTION_MODE_SPEED_BENCH;
    motion_test_armed = 0U;
    motion_zero_duty(&motion_test_duty);
    motion_test_remaining_ms = 0;
    interrupt_global_enable(primask);

    motion_apply_duty(0, 0, 0, 0);
    return AI_OK;
}

ai_status_t motion_speed_bench_stop(void)
{
    motion_mode = MOTION_MODE_ACTION_CLOSED_LOOP;
    motion_stop_drive();
    return AI_OK;
}

uint8_t motion_speed_bench_is_armed(void)
{
    return (motion_mode == MOTION_MODE_SPEED_BENCH) ? 1U : 0U;
}

ai_status_t motion_speed_bench_set_wheel(uint8_t wheel_id, float target_mm_s)
{
    if((wheel_id < 1U) || (wheel_id > (uint8_t)DRIVE_WHEEL_COUNT))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(motion_speed_bench_is_armed() == 0U)
    {
        return AI_ERR_BUSY;
    }

    if(wheel_id == 1U)
    {
        motion_speed_targets.wheel1 = target_mm_s;
    }
    else if(wheel_id == 2U)
    {
        motion_speed_targets.wheel2 = target_mm_s;
    }
    else if(wheel_id == 3U)
    {
        motion_speed_targets.wheel3 = target_mm_s;
    }
    else
    {
        motion_speed_targets.wheel4 = target_mm_s;
    }

    return motion_speed_set_targets(&motion_speed_controller,
                                    motion_speed_targets.wheel1,
                                    motion_speed_targets.wheel2,
                                    motion_speed_targets.wheel3,
                                    motion_speed_targets.wheel4);
}

ai_status_t motion_speed_bench_set_all(float wheel1_mm_s,
                                       float wheel2_mm_s,
                                       float wheel3_mm_s,
                                       float wheel4_mm_s)
{
    if(motion_speed_bench_is_armed() == 0U)
    {
        return AI_ERR_BUSY;
    }

    motion_speed_targets.wheel1 = wheel1_mm_s;
    motion_speed_targets.wheel2 = wheel2_mm_s;
    motion_speed_targets.wheel3 = wheel3_mm_s;
    motion_speed_targets.wheel4 = wheel4_mm_s;

    return motion_speed_set_targets(&motion_speed_controller,
                                    wheel1_mm_s,
                                    wheel2_mm_s,
                                    wheel3_mm_s,
                                    wheel4_mm_s);
}

ai_status_t motion_speed_bench_set_pid(float kp, float ki, float kd)
{
    if((kp < 0.0f) || (ki < 0.0f) || (kd < 0.0f))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_config.kp = kp;
    motion_speed_config.ki = ki;
    motion_speed_config.kd = kd;

    return motion_speed_set_config(&motion_speed_controller, &motion_speed_config);
}

ai_status_t motion_speed_bench_set_limits(float duty_limit_percent, float max_speed_mm_s)
{
    if((duty_limit_percent <= 0.0f) ||
       (duty_limit_percent > (float)DRIVE_DUTY_PERCENT_MAX) ||
       (max_speed_mm_s <= 0.0f))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_config.duty_limit_percent = duty_limit_percent;
    motion_speed_config.max_speed_mm_s = max_speed_mm_s;

    return motion_speed_set_config(&motion_speed_controller, &motion_speed_config);
}

ai_status_t motion_speed_bench_set_static(float duty_percent, float threshold_mm_s)
{
    if((duty_percent < 0.0f) ||
       (duty_percent > (float)DRIVE_DUTY_PERCENT_MAX) ||
       (threshold_mm_s < 0.0f))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_config.static_duty_percent = duty_percent;
    motion_speed_config.static_threshold_mm_s = threshold_mm_s;

    return motion_speed_set_config(&motion_speed_controller, &motion_speed_config);
}

ai_status_t motion_speed_bench_set_feedforward(float duty_per_mm_s)
{
    if(duty_per_mm_s < 0.0f)
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_config.feedforward_duty_per_mm_s = duty_per_mm_s;

    return motion_speed_set_config(&motion_speed_controller, &motion_speed_config);
}

ai_status_t motion_speed_bench_set_filter(float tau_ms)
{
    if(tau_ms < 0.0f)
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_speed_config.speed_filter_tau_ms = tau_ms;

    return motion_speed_set_config(&motion_speed_controller, &motion_speed_config);
}

void motion_speed_bench_get_sample(motion_speed_sample_t *sample)
{
    if(sample == 0)
    {
        return;
    }

    if(motion_mode == MOTION_MODE_ACTION_CLOSED_LOOP)
    {
        motion_action_runtime_get_speed_sample(sample);
    }
    else
    {
        *sample = motion_speed_last_sample;
    }
}

ai_status_t motion_action_begin(uint8_t cmd, uint8_t dir, uint8_t val)
{
    if(motion_mode != MOTION_MODE_ACTION_CLOSED_LOOP)
    {
        return AI_ERR_BUSY;
    }

    motion_mode = MOTION_MODE_ACTION_CLOSED_LOOP;
    return motion_action_runtime_begin(cmd, dir, val);
}

void motion_action_stop_all(void)
{
    motion_mode = MOTION_MODE_ACTION_CLOSED_LOOP;
    motion_test_armed = 0U;
    motion_action_runtime_stop_all();
    motion_stop_drive();
}

void motion_action_reset(void)
{
    motion_action_stop_all();
}

uint8_t motion_action_take_result(motion_action_result_t *result)
{
    return motion_action_runtime_take_result(result);
}

void motion_get_action_debug(motion_action_debug_t *debug)
{
    motion_action_runtime_get_debug(debug);
}
