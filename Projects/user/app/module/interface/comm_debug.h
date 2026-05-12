#ifndef COMM_DEBUG_H
#define COMM_DEBUG_H

#include <stdint.h>

#include "ai_error.h"

typedef void (*comm_debug_write_line_t)(const char *text);

void comm_debug_send_line(comm_debug_write_line_t write_line, const char *format, ...);
ai_status_t comm_debug_parse_i32(const char *text, int32_t *value);
ai_status_t comm_debug_parse_f32(const char *text, float *value);
ai_status_t comm_debug_parse_u8_auto(const char *text, uint8_t *value);

#endif
