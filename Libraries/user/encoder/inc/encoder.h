#ifndef USER_LIBRARIES_ENCODER_H
#define USER_LIBRARIES_ENCODER_H

#include "ai_error.h"

ai_status_t EncoderInit(void);
ai_status_t EncoderGetSpeed(int16_t *left, int16_t *right);

#endif
