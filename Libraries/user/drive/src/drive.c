#include "drive.h"

#include "drive_config.h"
#include "zf_common_interrupt.h"

typedef struct
{
    gpio_pin_enum dirPin;
    pwm_channel_enum pwmChannel;
} DriveMotorHardware;

typedef struct
{
    encoder_index_enum encoder;
    encoder_channel1_enum ch1Pin;
    encoder_channel2_enum ch2Pin;
    int16_t sign;
} DriveEncoderHardware;

static const DriveMotorHardware driveMotorHardware[DRIVE_WHEEL_COUNT] =
{
    {DRIVE_WHEEL1_DIR_PIN, DRIVE_WHEEL1_PWM_CHANNEL},
    {DRIVE_WHEEL2_DIR_PIN, DRIVE_WHEEL2_PWM_CHANNEL},
    {DRIVE_WHEEL3_DIR_PIN, DRIVE_WHEEL3_PWM_CHANNEL},
    {DRIVE_WHEEL4_DIR_PIN, DRIVE_WHEEL4_PWM_CHANNEL},
};

static const DriveEncoderHardware driveEncoderHardware[DRIVE_WHEEL_COUNT] =
{
    {DRIVE_WHEEL1_ENCODER, DRIVE_WHEEL1_ENCODER_CH1, DRIVE_WHEEL1_ENCODER_CH2, DRIVE_WHEEL1_ENCODER_SIGN},
    {DRIVE_WHEEL2_ENCODER, DRIVE_WHEEL2_ENCODER_CH1, DRIVE_WHEEL2_ENCODER_CH2, DRIVE_WHEEL2_ENCODER_SIGN},
    {DRIVE_WHEEL3_ENCODER, DRIVE_WHEEL3_ENCODER_CH1, DRIVE_WHEEL3_ENCODER_CH2, DRIVE_WHEEL3_ENCODER_SIGN},
    {DRIVE_WHEEL4_ENCODER, DRIVE_WHEEL4_ENCODER_CH1, DRIVE_WHEEL4_ENCODER_CH2, DRIVE_WHEEL4_ENCODER_SIGN},
};

static volatile int16_t driveEncoderDelta[DRIVE_WHEEL_COUNT];

static uint16_t DriveDutyPercentToPwm(int16_t dutyPercent)
{
    int32_t absoluteDuty = dutyPercent;

    if(absoluteDuty < 0)
    {
        absoluteDuty = -absoluteDuty;
    }

    if(absoluteDuty > DRIVE_DUTY_PERCENT_MAX)
    {
        absoluteDuty = DRIVE_DUTY_PERCENT_MAX;
    }

    return (uint16_t)((uint32_t)absoluteDuty * (PWM_DUTY_MAX / 100U));
}

static int16_t DriveApplyEncoderSign(int16_t count, int16_t sign)
{
    int32_t signedCount = count;

    if(sign < 0)
    {
        signedCount = -signedCount;
    }

    if(signedCount > INT16_MAX)
    {
        return INT16_MAX;
    }

    if(signedCount < INT16_MIN)
    {
        return INT16_MIN;
    }

    return (int16_t)signedCount;
}

DriveStatus DriveInit(void)
{
    for(uint8_t i = 0; i < (uint8_t)DRIVE_WHEEL_COUNT; i++)
    {
        gpio_init(driveMotorHardware[i].dirPin, GPO, DRIVE_DEFAULT_DIR_LEVEL, GPO_PUSH_PULL);
        pwm_init(driveMotorHardware[i].pwmChannel, DRIVE_PWM_FREQ_HZ, 0);

        encoder_quad_init(driveEncoderHardware[i].encoder,
                          driveEncoderHardware[i].ch1Pin,
                          driveEncoderHardware[i].ch2Pin);
        encoder_clear_count(driveEncoderHardware[i].encoder);
        driveEncoderDelta[i] = 0;
    }

    pit_ms_init(DRIVE_ENCODER_PIT_CHANNEL, DRIVE_ENCODER_SAMPLE_PERIOD_MS);

    return DRIVE_STATUS_OK;
}

DriveStatus DriveSetDuty(DriveWheelId id, int16_t dutyPercent)
{
    uint8_t direction = DRIVE_FORWARD_LEVEL;

    if((id < DRIVE_WHEEL_1) || (id >= DRIVE_WHEEL_COUNT))
    {
        return DRIVE_STATUS_INVALID_ARG;
    }

    if(dutyPercent < 0)
    {
        direction = DRIVE_REVERSE_LEVEL;
    }

    gpio_set_level(driveMotorHardware[id].dirPin, direction);
    pwm_set_duty(driveMotorHardware[id].pwmChannel, DriveDutyPercentToPwm(dutyPercent));

    return DRIVE_STATUS_OK;
}

DriveStatus DriveSetAllDuty(int16_t wheel1Duty,
                            int16_t wheel2Duty,
                            int16_t wheel3Duty,
                            int16_t wheel4Duty)
{
    (void)DriveSetDuty(DRIVE_WHEEL_1, wheel1Duty);
    (void)DriveSetDuty(DRIVE_WHEEL_2, wheel2Duty);
    (void)DriveSetDuty(DRIVE_WHEEL_3, wheel3Duty);
    (void)DriveSetDuty(DRIVE_WHEEL_4, wheel4Duty);

    return DRIVE_STATUS_OK;
}

void DriveStopAll(void)
{
    (void)DriveSetAllDuty(0, 0, 0, 0);
}

DriveStatus DriveGetEncoderDelta(DriveWheelId id, int16_t *delta)
{
    uint32_t primask;

    if((id < DRIVE_WHEEL_1) || (id >= DRIVE_WHEEL_COUNT) || (delta == 0))
    {
        return DRIVE_STATUS_INVALID_ARG;
    }

    primask = interrupt_global_disable();
    *delta = driveEncoderDelta[id];
    interrupt_global_enable(primask);

    return DRIVE_STATUS_OK;
}

DriveStatus DriveGetAllEncoderDelta(DriveEncoderDelta *delta)
{
    uint32_t primask;

    if(delta == 0)
    {
        return DRIVE_STATUS_INVALID_ARG;
    }

    primask = interrupt_global_disable();
    delta->wheel1 = driveEncoderDelta[DRIVE_WHEEL_1];
    delta->wheel2 = driveEncoderDelta[DRIVE_WHEEL_2];
    delta->wheel3 = driveEncoderDelta[DRIVE_WHEEL_3];
    delta->wheel4 = driveEncoderDelta[DRIVE_WHEEL_4];
    interrupt_global_enable(primask);

    return DRIVE_STATUS_OK;
}

void DriveEncoderPitHandler(void)
{
    for(uint8_t i = 0; i < (uint8_t)DRIVE_WHEEL_COUNT; i++)
    {
        const encoder_index_enum encoder = driveEncoderHardware[i].encoder;
        const int16_t count = encoder_get_count(encoder);

        driveEncoderDelta[i] = DriveApplyEncoderSign(count, driveEncoderHardware[i].sign);
        encoder_clear_count(encoder);
    }
}
