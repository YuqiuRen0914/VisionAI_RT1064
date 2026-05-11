#include "mecanum.h"

#define MECANUM_MOTOR1_SIGN    (1)
#define MECANUM_MOTOR2_SIGN    (1)
#define MECANUM_MOTOR3_SIGN    (1)
#define MECANUM_MOTOR4_SIGN    (1)

static int32_t mecanum_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int16_t mecanum_scale_i16(int16_t value, int32_t limit, int32_t max_abs)
{
    return (int16_t)(((int32_t)value * limit) / max_abs);
}

static int16_t mecanum_saturate_i16(int32_t value)
{
    if(value > INT16_MAX)
    {
        return INT16_MAX;
    }

    if(value < INT16_MIN)
    {
        return INT16_MIN;
    }

    return (int16_t)value;
}

ai_status_t mecanum_solve_duty(const mecanum_velocity_t *velocity, mecanum_duty_t *duty)
{
    if((velocity == 0) || (duty == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    duty->motor1 = mecanum_saturate_i16(((int32_t)velocity->vx - velocity->vy - velocity->wz) * MECANUM_MOTOR1_SIGN);
    duty->motor2 = mecanum_saturate_i16(((int32_t)velocity->vx + velocity->vy + velocity->wz) * MECANUM_MOTOR2_SIGN);
    duty->motor3 = mecanum_saturate_i16(((int32_t)velocity->vx - velocity->vy + velocity->wz) * MECANUM_MOTOR3_SIGN);
    duty->motor4 = mecanum_saturate_i16(((int32_t)velocity->vx + velocity->vy - velocity->wz) * MECANUM_MOTOR4_SIGN);

    return AI_OK;
}

ai_status_t mecanum_normalize_duty(mecanum_duty_t *duty, int16_t limit)
{
    int32_t max_abs;

    if((duty == 0) || (limit <= 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    max_abs = mecanum_abs_i32(duty->motor1);

    if(mecanum_abs_i32(duty->motor2) > max_abs)
    {
        max_abs = mecanum_abs_i32(duty->motor2);
    }

    if(mecanum_abs_i32(duty->motor3) > max_abs)
    {
        max_abs = mecanum_abs_i32(duty->motor3);
    }

    if(mecanum_abs_i32(duty->motor4) > max_abs)
    {
        max_abs = mecanum_abs_i32(duty->motor4);
    }

    if(max_abs <= limit)
    {
        return AI_OK;
    }

    duty->motor1 = mecanum_scale_i16(duty->motor1, limit, max_abs);
    duty->motor2 = mecanum_scale_i16(duty->motor2, limit, max_abs);
    duty->motor3 = mecanum_scale_i16(duty->motor3, limit, max_abs);
    duty->motor4 = mecanum_scale_i16(duty->motor4, limit, max_abs);

    return AI_OK;
}
