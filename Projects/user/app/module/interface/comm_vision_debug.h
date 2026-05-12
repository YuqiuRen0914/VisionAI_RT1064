#ifndef COMM_VISION_DEBUG_H
#define COMM_VISION_DEBUG_H

typedef void (*comm_vision_debug_write_line_t)(const char *text);

void comm_vision_debug_handle(char *sub_command, comm_vision_debug_write_line_t write_line);

#endif
