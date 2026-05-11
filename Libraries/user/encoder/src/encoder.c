#include "encoder.h"

ai_status_t EncoderInit(void)
{
    return AI_OK;
}

ai_status_t EncoderGetSpeed(int16_t *left, int16_t *right)
{
    if((left == NULL) || (right == NULL))
    {
        return AI_ERR_INVALID_ARG;
    }

    *left = 0;
    *right = 0;
    return AI_OK;
}
