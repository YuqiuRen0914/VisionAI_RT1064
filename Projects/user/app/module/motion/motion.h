#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>

#include "ai_error.h"
#include "mecanum.h"

ai_status_t motion_module_init(void);
void motion_module_tick(void);

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

#endif
