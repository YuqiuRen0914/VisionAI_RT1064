#include "camera_mt9v03x.h"

ai_status_t CameraMt9v03xInit(void)
{
    return AI_OK;
}

ai_status_t CameraMt9v03xGetFrame(CameraMt9v03xFrame *frame)
{
    if(frame == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    frame->data = NULL;
    frame->width = 0;
    frame->height = 0;

    return AI_ERR_NO_DATA;
}
