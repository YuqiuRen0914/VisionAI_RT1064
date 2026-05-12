#include "comm_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define COMM_DEBUG_RESPONSE_BUFFER_SIZE (384U)

void comm_debug_send_line(comm_debug_write_line_t write_line, const char *format, ...)
{
    char buffer[COMM_DEBUG_RESPONSE_BUFFER_SIZE];
    va_list args;
    int length;

    if((write_line == 0) || (format == 0))
    {
        return;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if(length < 0)
    {
        return;
    }

    if((uint32_t)length >= sizeof(buffer))
    {
        length = (int)(sizeof(buffer) - 1U);
        buffer[length] = '\0';
    }

    write_line(buffer);
}

ai_status_t comm_debug_parse_i32(const char *text, int32_t *value)
{
    char *end_ptr;
    long parsed;

    if((text == 0) || (value == 0) || (text[0] == '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    parsed = strtol(text, &end_ptr, 10);
    if((end_ptr == text) || (*end_ptr != '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    *value = (int32_t)parsed;
    return AI_OK;
}

ai_status_t comm_debug_parse_f32(const char *text, float *value)
{
    char *end_ptr;
    float parsed;

    if((text == 0) || (value == 0) || (text[0] == '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    parsed = strtof(text, &end_ptr);
    if((end_ptr == text) || (*end_ptr != '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    *value = parsed;
    return AI_OK;
}

ai_status_t comm_debug_parse_u8_auto(const char *text, uint8_t *value)
{
    char *end_ptr;
    unsigned long parsed;

    if((text == 0) || (value == 0) || (text[0] == '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    parsed = strtoul(text, &end_ptr, 0);
    if((end_ptr == text) || (*end_ptr != '\0') || (parsed > UINT8_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    *value = (uint8_t)parsed;
    return AI_OK;
}
