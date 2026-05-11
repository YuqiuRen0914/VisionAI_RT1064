#ifndef USER_LIBRARIES_MOTOR_CONFIG_H
#define USER_LIBRARIES_MOTOR_CONFIG_H

#include "zf_driver_gpio.h"
#include "zf_driver_pwm.h"

#define MOTOR_PWM_FREQ_HZ             (17000U)
#define MOTOR_DUTY_PERCENT_MAX        (100)

#define MOTOR_FORWARD_LEVEL           (GPIO_HIGH)
#define MOTOR_REVERSE_LEVEL           (GPIO_LOW)
#define MOTOR_DEFAULT_DIR_LEVEL       (MOTOR_FORWARD_LEVEL)

#define MOTOR1_DIR_PIN                (C9)
#define MOTOR1_PWM_CHANNEL            (PWM2_MODULE1_CHA_C8)

#define MOTOR2_DIR_PIN                (C7)
#define MOTOR2_PWM_CHANNEL            (PWM2_MODULE0_CHA_C6)

#define MOTOR3_DIR_PIN                (D2)
#define MOTOR3_PWM_CHANNEL            (PWM2_MODULE3_CHB_D3)

#define MOTOR4_DIR_PIN                (C10)
#define MOTOR4_PWM_CHANNEL            (PWM2_MODULE2_CHB_C11)

#endif
