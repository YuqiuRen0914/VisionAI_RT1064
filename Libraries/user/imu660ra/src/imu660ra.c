#include "imu660ra.h"

#include "imu660ra_config.h"
#include "zf_common_interrupt.h"
#include "zf_device_imu660ra.h"

static volatile Imu660raRawData imu660raRawData;
static volatile float imu660raYawDeg;
static volatile uint8_t imu660raInitialized;

static void Imu660raCopyRaw(Imu660raRawData *data)
{
    data->accX = imu660raRawData.accX;
    data->accY = imu660raRawData.accY;
    data->accZ = imu660raRawData.accZ;
    data->gyroX = imu660raRawData.gyroX;
    data->gyroY = imu660raRawData.gyroY;
    data->gyroZ = imu660raRawData.gyroZ;
}

static void Imu660raRawToScaled(const Imu660raRawData *raw, float yawDeg, Imu660raScaledData *scaled)
{
    scaled->accXG = imu660ra_acc_transition(raw->accX);
    scaled->accYG = imu660ra_acc_transition(raw->accY);
    scaled->accZG = imu660ra_acc_transition(raw->accZ);
    scaled->gyroXDps = imu660ra_gyro_transition(raw->gyroX);
    scaled->gyroYDps = imu660ra_gyro_transition(raw->gyroY);
    scaled->gyroZDps = imu660ra_gyro_transition(raw->gyroZ);
    scaled->yawDeg = yawDeg;
}

ai_status_t Imu660raInit(void)
{
    uint32_t primask;

    primask = interrupt_global_disable();
    imu660raRawData.accX = 0;
    imu660raRawData.accY = 0;
    imu660raRawData.accZ = 0;
    imu660raRawData.gyroX = 0;
    imu660raRawData.gyroY = 0;
    imu660raRawData.gyroZ = 0;
    imu660raYawDeg = 0.0f;
    imu660raInitialized = 0;
    interrupt_global_enable(primask);

    if(imu660ra_init() != 0)
    {
        return AI_ERR;
    }

    imu660ra_get_acc();
    imu660ra_get_gyro();

    primask = interrupt_global_disable();
    imu660raRawData.accX = imu660ra_acc_x;
    imu660raRawData.accY = imu660ra_acc_y;
    imu660raRawData.accZ = imu660ra_acc_z;
    imu660raRawData.gyroX = imu660ra_gyro_x;
    imu660raRawData.gyroY = imu660ra_gyro_y;
    imu660raRawData.gyroZ = imu660ra_gyro_z;
    imu660raYawDeg = 0.0f;
    imu660raInitialized = 1;
    interrupt_global_enable(primask);

    pit_ms_init(IMU660RA_SAMPLE_PIT_CHANNEL, IMU660RA_SAMPLE_PERIOD_MS);

    return AI_OK;
}

ai_status_t Imu660raGetRaw(Imu660raRawData *data)
{
    uint32_t primask;

    if(data == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(imu660raInitialized == 0)
    {
        return AI_ERR_NO_DATA;
    }

    primask = interrupt_global_disable();
    Imu660raCopyRaw(data);
    interrupt_global_enable(primask);

    return AI_OK;
}

ai_status_t Imu660raGetScaled(Imu660raScaledData *data)
{
    uint32_t primask;
    Imu660raRawData raw;
    float yawDeg;

    if(data == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(imu660raInitialized == 0)
    {
        return AI_ERR_NO_DATA;
    }

    primask = interrupt_global_disable();
    Imu660raCopyRaw(&raw);
    yawDeg = imu660raYawDeg;
    interrupt_global_enable(primask);

    Imu660raRawToScaled(&raw, yawDeg, data);

    return AI_OK;
}

ai_status_t Imu660raGetYawDeg(float *yawDeg)
{
    uint32_t primask;

    if(yawDeg == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(imu660raInitialized == 0)
    {
        return AI_ERR_NO_DATA;
    }

    primask = interrupt_global_disable();
    *yawDeg = imu660raYawDeg;
    interrupt_global_enable(primask);

    return AI_OK;
}

void Imu660raResetYaw(void)
{
    uint32_t primask = interrupt_global_disable();

    imu660raYawDeg = 0.0f;
    interrupt_global_enable(primask);
}

void Imu660raPitHandler(void)
{
    if(imu660raInitialized == 0)
    {
        return;
    }

    imu660ra_get_acc();
    imu660ra_get_gyro();

    imu660raRawData.accX = imu660ra_acc_x;
    imu660raRawData.accY = imu660ra_acc_y;
    imu660raRawData.accZ = imu660ra_acc_z;
    imu660raRawData.gyroX = imu660ra_gyro_x;
    imu660raRawData.gyroY = imu660ra_gyro_y;
    imu660raRawData.gyroZ = imu660ra_gyro_z;
    imu660raYawDeg += imu660ra_gyro_transition(imu660raRawData.gyroX) *
                      IMU660RA_BODY_YAW_AXIS_SIGN *
                      ((float)IMU660RA_SAMPLE_PERIOD_MS / 1000.0f);
}
