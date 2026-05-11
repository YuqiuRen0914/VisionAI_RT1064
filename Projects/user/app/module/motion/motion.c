#include "motion.h"

#include "ai_config.h"
#include "drive.h"
#include "drive_config.h"
#include "event.h"
#include "mecanum.h"

static int16_t motion_line_offset;
static int16_t motion_target_speed;

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

ai_status_t motion_module_init(void)
{
    motion_line_offset = 0;
    motion_target_speed = 0;
#if AI_MOTOR_TEST_ENABLE
    motor_test_elapsed_ms = 0;
    motor_test_phase = 0;
#endif

    return motion_drive_status_to_ai(DriveInit());
}

void motion_module_tick(void)
{
#if AI_MOTOR_TEST_ENABLE
    motion_motor_test_tick();
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
