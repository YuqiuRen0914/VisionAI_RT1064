#ifndef USER_LIBRARIES_IMU660RA_H
#define USER_LIBRARIES_IMU660RA_H

#include "ai_error.h"

typedef struct
{
    int16_t accX;
    int16_t accY;
    int16_t accZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} Imu660raRawData;

typedef struct
{
    float accXG;
    float accYG;
    float accZG;
    float gyroXDps;
    float gyroYDps;
    float gyroZDps;
    float yawDeg;
} Imu660raScaledData;

ai_status_t Imu660raInit(void);
ai_status_t Imu660raGetRaw(Imu660raRawData *data);
ai_status_t Imu660raGetScaled(Imu660raScaledData *data);
ai_status_t Imu660raGetYawDeg(float *yawDeg);
void Imu660raResetYaw(void);
void Imu660raPitHandler(void);

#endif
