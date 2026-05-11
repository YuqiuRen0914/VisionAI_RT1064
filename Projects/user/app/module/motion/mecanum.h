#ifndef MECANUM_H
#define MECANUM_H

#include <stdint.h>

#include "ai_error.h"

typedef struct
{
    int16_t vx;
    int16_t vy;
    int16_t wz;
} mecanum_velocity_t;

typedef struct
{
    int16_t motor1;
    int16_t motor2;
    int16_t motor3;
    int16_t motor4;
} mecanum_duty_t;

ai_status_t mecanum_solve_duty(const mecanum_velocity_t *velocity, mecanum_duty_t *duty);
ai_status_t mecanum_normalize_duty(mecanum_duty_t *duty, int16_t limit);

#endif
