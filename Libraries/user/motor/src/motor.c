#include "motor.h"

#include "motor_config.h"

typedef struct
{
    gpio_pin_enum dirPin;
    pwm_channel_enum pwmChannel;
} MotorHardware;

static const MotorHardware motorHardware[MOTOR_ID_COUNT] =
{
    {MOTOR1_DIR_PIN, MOTOR1_PWM_CHANNEL},
    {MOTOR2_DIR_PIN, MOTOR2_PWM_CHANNEL},
    {MOTOR3_DIR_PIN, MOTOR3_PWM_CHANNEL},
    {MOTOR4_DIR_PIN, MOTOR4_PWM_CHANNEL},
};

static uint16_t MotorDutyPercentToPwm(int16_t dutyPercent)
{
    int32_t absoluteDuty = dutyPercent;

    if(absoluteDuty < 0)
    {
        absoluteDuty = -absoluteDuty;
    }

    if(absoluteDuty > MOTOR_DUTY_PERCENT_MAX)
    {
        absoluteDuty = MOTOR_DUTY_PERCENT_MAX;
    }

    return (uint16_t)((uint32_t)absoluteDuty * (PWM_DUTY_MAX / 100U));
}

MotorStatus MotorInit(void)
{
    for(uint8_t i = 0; i < (uint8_t)MOTOR_ID_COUNT; i++)
    {
        gpio_init(motorHardware[i].dirPin, GPO, MOTOR_DEFAULT_DIR_LEVEL, GPO_PUSH_PULL);
        pwm_init(motorHardware[i].pwmChannel, MOTOR_PWM_FREQ_HZ, 0);
    }

    return MOTOR_STATUS_OK;
}

MotorStatus MotorSetDuty(MotorId id, int16_t dutyPercent)
{
    uint8_t direction = MOTOR_FORWARD_LEVEL;

    if((id < MOTOR_ID_1) || (id >= MOTOR_ID_COUNT))
    {
        return MOTOR_STATUS_INVALID_ARG;
    }

    if(dutyPercent < 0)
    {
        direction = MOTOR_REVERSE_LEVEL;
    }

    gpio_set_level(motorHardware[id].dirPin, direction);
    pwm_set_duty(motorHardware[id].pwmChannel, MotorDutyPercentToPwm(dutyPercent));

    return MOTOR_STATUS_OK;
}

MotorStatus MotorSetAllDuty(int16_t motor1Duty,
                            int16_t motor2Duty,
                            int16_t motor3Duty,
                            int16_t motor4Duty)
{
    (void)MotorSetDuty(MOTOR_ID_1, motor1Duty);
    (void)MotorSetDuty(MOTOR_ID_2, motor2Duty);
    (void)MotorSetDuty(MOTOR_ID_3, motor3Duty);
    (void)MotorSetDuty(MOTOR_ID_4, motor4Duty);

    return MOTOR_STATUS_OK;
}

MotorStatus MotorSetLeftRightDuty(int16_t leftDuty, int16_t rightDuty)
{
    return MotorSetAllDuty(rightDuty, leftDuty, leftDuty, rightDuty);
}

void MotorStopAll(void)
{
    (void)MotorSetAllDuty(0, 0, 0, 0);
}
