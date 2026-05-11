#include "motion.h"

#include "ai_config.h"
#include "drive.h"
#include "drive_config.h"
#include "event.h"
#include "imu660ra.h"
#include "mecanum.h"
#include "zf_common_interrupt.h"

static int16_t motion_line_offset;
static int16_t motion_target_speed;

#if AI_OPEN_LOOP_TEST_ENABLE
static mecanum_duty_t motion_test_duty;
static uint32_t motion_test_remaining_ms;
static volatile uint8_t motion_test_armed;
#endif

static ai_status_t motion_drive_status_to_ai(DriveStatus status)
{
    return (status == DRIVE_STATUS_OK) ? AI_OK : AI_ERR;
}

#if AI_MOTOR_TEST_ENABLE
static uint32_t motor_test_elapsed_ms;
static uint8_t motor_test_phase;

static void motion_motor_test_tick(void)
{
    const uint32_t run_ticks = AI_MOTOR_TEST_RUN_MS / AI_MOTION_PERIOD_MS;
    const uint32_t stop_ticks = AI_MOTOR_TEST_STOP_MS / AI_MOTION_PERIOD_MS;
    const uint32_t phase_ticks = run_ticks + stop_ticks;
    DriveWheelId wheel_id = (DriveWheelId)(motor_test_phase % (uint8_t)DRIVE_WHEEL_COUNT);

    DriveStopAll();

    if(motor_test_elapsed_ms < run_ticks)
    {
        (void)DriveSetDuty(wheel_id, AI_MOTOR_TEST_DUTY_PERCENT);
    }

    motor_test_elapsed_ms++;

    if(motor_test_elapsed_ms >= phase_ticks)
    {
        motor_test_elapsed_ms = 0;
        motor_test_phase = (uint8_t)((motor_test_phase + 1U) % (uint8_t)DRIVE_WHEEL_COUNT);
    }
}
#endif

#if AI_OPEN_LOOP_TEST_ENABLE
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

static void motion_test_apply_locked(const mecanum_duty_t *duty, uint32_t run_ms)
{
    uint32_t primask = interrupt_global_disable();

    motion_test_duty = *duty;
    motion_test_remaining_ms = run_ms;

    if(motion_test_remaining_ms == 0U)
    {
        motion_test_duty.motor1 = 0;
        motion_test_duty.motor2 = 0;
        motion_test_duty.motor3 = 0;
        motion_test_duty.motor4 = 0;
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
        motion_test_duty.motor1 = 0;
        motion_test_duty.motor2 = 0;
        motion_test_duty.motor3 = 0;
        motion_test_duty.motor4 = 0;
    }
    else if(motion_test_remaining_ms > AI_MOTION_PERIOD_MS)
    {
        motion_test_remaining_ms -= AI_MOTION_PERIOD_MS;
    }
    else if(motion_test_remaining_ms != 0U)
    {
        motion_test_remaining_ms = 0;
        motion_test_duty.motor1 = 0;
        motion_test_duty.motor2 = 0;
        motion_test_duty.motor3 = 0;
        motion_test_duty.motor4 = 0;
    }

    duty = motion_test_duty;
    interrupt_global_enable(primask);

    (void)DriveSetAllDuty(duty.motor1,
                          duty.motor2,
                          duty.motor3,
                          duty.motor4);
}
#endif

ai_status_t motion_module_init(void)
{
    ai_status_t status;

    motion_line_offset = 0;
    motion_target_speed = 0;
#if AI_MOTOR_TEST_ENABLE
    motor_test_elapsed_ms = 0;
    motor_test_phase = 0;
#endif
#if AI_OPEN_LOOP_TEST_ENABLE
    motion_test_duty.motor1 = 0;
    motion_test_duty.motor2 = 0;
    motion_test_duty.motor3 = 0;
    motion_test_duty.motor4 = 0;
    motion_test_remaining_ms = 0;
    motion_test_armed = 0;
#endif

    status = motion_drive_status_to_ai(DriveInit());
    if(status != AI_OK)
    {
        return status;
    }

    return Imu660raInit();
}

void motion_module_tick(void)
{
#if AI_MOTOR_TEST_ENABLE
    motion_motor_test_tick();
#elif AI_OPEN_LOOP_TEST_ENABLE
    motion_test_tick();
#else
    event_msg_t event;
    mecanum_duty_t duty;
    mecanum_velocity_t velocity;

    while(event_poll(&event) == AI_OK)
    {
        if(event.id == EVENT_ID_VISION_RESULT)
        {
            motion_line_offset = (int16_t)event.value0;
            motion_target_speed = (int16_t)event.value1;
        }
    }

    velocity.vx = motion_target_speed;
    velocity.vy = 0;
    velocity.wz = (int16_t)-motion_line_offset;

    if((mecanum_solve_duty(&velocity, &duty) == AI_OK) &&
       (mecanum_normalize_duty(&duty, DRIVE_DUTY_PERCENT_MAX) == AI_OK))
    {
        (void)DriveSetAllDuty(duty.motor1,
                              duty.motor2,
                              duty.motor3,
                              duty.motor4);
    }
#endif
}

#if AI_OPEN_LOOP_TEST_ENABLE
ai_status_t motion_test_arm(void)
{
    uint32_t primask = interrupt_global_disable();

    motion_test_duty.motor1 = 0;
    motion_test_duty.motor2 = 0;
    motion_test_duty.motor3 = 0;
    motion_test_duty.motor4 = 0;
    motion_test_remaining_ms = 0;
    motion_test_armed = 1U;
    interrupt_global_enable(primask);

    DriveStopAll();

    return AI_OK;
}

ai_status_t motion_test_disarm(void)
{
    return motion_test_stop();
}

uint8_t motion_test_is_armed(void)
{
    return motion_test_armed;
}

ai_status_t motion_test_stop(void)
{
    uint32_t primask = interrupt_global_disable();

    motion_test_duty.motor1 = 0;
    motion_test_duty.motor2 = 0;
    motion_test_duty.motor3 = 0;
    motion_test_duty.motor4 = 0;
    motion_test_remaining_ms = 0;
    motion_test_armed = 0;
    interrupt_global_enable(primask);

    DriveStopAll();

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

    if(motion_test_armed == 0U)
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

    if(motion_test_armed == 0U)
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

    if(motion_test_armed == 0U)
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
    *duty = motion_test_duty;
    interrupt_global_enable(primask);
}
#endif
