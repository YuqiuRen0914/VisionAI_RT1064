#include "log.h"

#include "wireless.h"

void log_service_init(void)
{
}

ai_status_t log_write(const char *text)
{
    const char *cursor = text;
    uint32_t length = 0;

    if(text == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    while(*cursor++ != '\0')
    {
        length++;
    }

    return WirelessSend((const uint8_t *)text, length);
}
