#include "motion.h"

#include "ai_config.h"
#include "encoder.h"
#include "event.h"
#include "motor.h"

static int16_t motion_line_offset;
static int16_t motion_target_speed;

static ai_status_t motion_motor_status_to_ai(MotorStatus status)
{
    return (status == MOTOR_STATUS_OK) ? AI_OK : AI_ERR;
}

#if AI_MOTOR_TEST_ENABLE
static uint32_t motor_test_elapsed_ms;
static uint8_t motor_test_phase;

static void motion_motor_test_tick(void)
{
    const uint32_t run_ticks = AI_MOTOR_TEST_RUN_MS / AI_MOTION_PERIOD_MS;
    const uint32_t stop_ticks = AI_MOTOR_TEST_STOP_MS / AI_MOTION_PERIOD_MS;
    const uint32_t phase_ticks = run_ticks + stop_ticks;
    MotorId motor_id = (MotorId)(motor_test_phase % (uint8_t)MOTOR_ID_COUNT);

    MotorStopAll();

    if(motor_test_elapsed_ms < run_ticks)
    {
        (void)MotorSetDuty(motor_id, AI_MOTOR_TEST_DUTY_PERCENT);
    }

    motor_test_elapsed_ms++;

    if(motor_test_elapsed_ms >= phase_ticks)
    {
        motor_test_elapsed_ms = 0;
        motor_test_phase = (uint8_t)((motor_test_phase + 1U) % (uint8_t)MOTOR_ID_COUNT);
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

    (void)EncoderInit();
    return motion_motor_status_to_ai(MotorInit());
}

void motion_module_tick(void)
{
#if AI_MOTOR_TEST_ENABLE
    motion_motor_test_tick();
#else
    event_msg_t event;
    int16_t left_speed;
    int16_t right_speed;

    while(event_poll(&event) == AI_OK)
    {
        if(event.id == EVENT_ID_VISION_RESULT)
        {
            motion_line_offset = (int16_t)event.value0;
            motion_target_speed = (int16_t)event.value1;
        }
    }

    (void)EncoderGetSpeed(&left_speed, &right_speed);
    (void)left_speed;
    (void)right_speed;

    (void)MotorSetLeftRightDuty(motion_target_speed - motion_line_offset,
                                motion_target_speed + motion_line_offset);
#endif
}
