#include "wireless.h"

#include <stdio.h>

ai_status_t WirelessInit(void)
{
    return AI_OK;
}

ai_status_t WirelessSend(const uint8_t *data, uint32_t length)
{
    if(data == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    for(uint32_t i = 0; i < length; i++)
    {
        (void)printf("%c", data[i]);
    }

    return AI_OK;
}
