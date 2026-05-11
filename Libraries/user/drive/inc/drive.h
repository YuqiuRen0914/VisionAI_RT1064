#ifndef USER_LIBRARIES_DRIVE_H
#define USER_LIBRARIES_DRIVE_H

#include <stdint.h>

typedef enum
{
    DRIVE_STATUS_OK = 0,
    DRIVE_STATUS_INVALID_ARG = -1,
} DriveStatus;

typedef enum
{
    DRIVE_WHEEL_1 = 0,
    DRIVE_WHEEL_2,
    DRIVE_WHEEL_3,
    DRIVE_WHEEL_4,
    DRIVE_WHEEL_COUNT,
} DriveWheelId;

typedef struct
{
    int16_t wheel1;
    int16_t wheel2;
    int16_t wheel3;
    int16_t wheel4;
} DriveEncoderDelta;

typedef struct
{
    int32_t wheel1;
    int32_t wheel2;
    int32_t wheel3;
    int32_t wheel4;
} DriveEncoderTotal;

DriveStatus DriveInit(void);
DriveStatus DriveSetDuty(DriveWheelId id, int16_t dutyPercent);
DriveStatus DriveSetAllDuty(int16_t wheel1Duty,
                            int16_t wheel2Duty,
                            int16_t wheel3Duty,
                            int16_t wheel4Duty);
void DriveStopAll(void);

DriveStatus DriveGetEncoderDelta(DriveWheelId id, int16_t *delta);
DriveStatus DriveGetAllEncoderDelta(DriveEncoderDelta *delta);
DriveStatus DriveGetAllEncoderTotal(DriveEncoderTotal *total);
void DriveEncoderPitHandler(void);

#endif
