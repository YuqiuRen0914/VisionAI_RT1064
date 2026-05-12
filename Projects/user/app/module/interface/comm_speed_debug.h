#ifndef COMM_SPEED_DEBUG_H
#define COMM_SPEED_DEBUG_H

typedef void (*comm_speed_debug_write_line_t)(const char *text);

void comm_speed_debug_handle(char *sub_command, comm_speed_debug_write_line_t write_line);

#endif
