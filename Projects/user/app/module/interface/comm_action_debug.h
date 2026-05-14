#ifndef COMM_ACTION_DEBUG_H
#define COMM_ACTION_DEBUG_H

#include "comm_debug.h"

typedef comm_debug_write_line_t comm_action_debug_write_line_t;

void comm_action_debug_handle(char *sub_command, comm_action_debug_write_line_t write_line);

#endif
