#ifndef COMM_CALIBRATION_DEBUG_H
#define COMM_CALIBRATION_DEBUG_H

typedef void (*comm_calibration_debug_write_line_t)(const char *text);

void comm_calibration_debug_init(void);
void comm_calibration_debug_handle(char *sub_command, comm_calibration_debug_write_line_t write_line);

#endif
