#ifndef USER_LIBRARIES_CAMERA_MT9V03X_H
#define USER_LIBRARIES_CAMERA_MT9V03X_H

#include "ai_error.h"

typedef struct
{
    const uint8_t *data;
    uint16_t width;
    uint16_t height;
} CameraMt9v03xFrame;

ai_status_t CameraMt9v03xInit(void);
ai_status_t CameraMt9v03xGetFrame(CameraMt9v03xFrame *frame);

#endif
