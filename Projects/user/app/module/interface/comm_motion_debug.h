#ifndef COMM_MOTION_DEBUG_H
#define COMM_MOTION_DEBUG_H

typedef void (*comm_motion_debug_write_line_t)(const char *text);

void comm_motion_debug_handle_arm(comm_motion_debug_write_line_t write_line);
void comm_motion_debug_handle_disarm(comm_motion_debug_write_line_t write_line);
void comm_motion_debug_handle_motor(char *wheel_text, comm_motion_debug_write_line_t write_line);
void comm_motion_debug_handle_move(char *move_text, comm_motion_debug_write_line_t write_line);

#endif
