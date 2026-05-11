#ifndef USER_LIBRARIES_MOTOR_H
#define USER_LIBRARIES_MOTOR_H

#include <stdint.h>

typedef enum
{
    MOTOR_STATUS_OK = 0,
    MOTOR_STATUS_INVALID_ARG = -1,
} MotorStatus;

typedef enum
{
    MOTOR_ID_1 = 0,
    MOTOR_ID_2,
    MOTOR_ID_3,
    MOTOR_ID_4,
    MOTOR_ID_COUNT,
} MotorId;

MotorStatus MotorInit(void);
MotorStatus MotorSetDuty(MotorId id, int16_t dutyPercent);
MotorStatus MotorSetAllDuty(int16_t motor1Duty,
                            int16_t motor2Duty,
                            int16_t motor3Duty,
                            int16_t motor4Duty);
MotorStatus MotorSetLeftRightDuty(int16_t leftDuty, int16_t rightDuty);
void MotorStopAll(void);

#endif
