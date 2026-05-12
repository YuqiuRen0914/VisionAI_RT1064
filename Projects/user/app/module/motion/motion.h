#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>

#include "ai_error.h"
#include "mecanum.h"
#include "motion_speed.h"

typedef enum
{
    MOTION_MODE_ACTION_CLOSED_LOOP = 0,
    MOTION_MODE_OPEN_LOOP_TEST,
    MOTION_MODE_SPEED_BENCH,
} motion_mode_t;

ai_status_t motion_module_init(void);
void motion_module_tick(void);
motion_mode_t motion_get_mode(void);

ai_status_t motion_test_arm(void);
ai_status_t motion_test_disarm(void);
uint8_t motion_test_is_armed(void);
ai_status_t motion_test_stop(void);
ai_status_t motion_test_set_motor(uint8_t wheel_id, int16_t duty_percent, uint32_t run_ms);
ai_status_t motion_test_set_all(int16_t wheel1_duty,
                                int16_t wheel2_duty,
                                int16_t wheel3_duty,
                                int16_t wheel4_duty,
                                uint32_t run_ms);
ai_status_t motion_test_set_mecanum(const mecanum_velocity_t *velocity, uint32_t run_ms);
void motion_test_get_duty(mecanum_duty_t *duty);

ai_status_t motion_speed_bench_arm(void);
ai_status_t motion_speed_bench_stop(void);
uint8_t motion_speed_bench_is_armed(void);
ai_status_t motion_speed_bench_set_wheel(uint8_t wheel_id, float target_mm_s);
ai_status_t motion_speed_bench_set_all(float wheel1_mm_s,
                                       float wheel2_mm_s,
                                       float wheel3_mm_s,
                                       float wheel4_mm_s);
ai_status_t motion_speed_bench_set_pid(float kp, float ki, float kd);
ai_status_t motion_speed_bench_set_limits(float duty_limit_percent, float max_speed_mm_s);
ai_status_t motion_speed_bench_set_static(float duty_percent, float threshold_mm_s);
ai_status_t motion_speed_bench_set_feedforward(float duty_per_mm_s);
ai_status_t motion_speed_bench_set_filter(float tau_ms);
void motion_speed_bench_get_sample(motion_speed_sample_t *sample);

ai_status_t motion_action_begin(uint8_t cmd, uint8_t dir, uint8_t val);
void motion_action_stop_all(void);
void motion_action_reset(void);

#endif
