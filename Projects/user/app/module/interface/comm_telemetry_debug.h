#ifndef COMM_TELEMETRY_DEBUG_H
#define COMM_TELEMETRY_DEBUG_H

typedef void (*comm_telemetry_debug_write_line_t)(const char *text);

void comm_telemetry_debug_init(void);
const char *comm_telemetry_debug_stream_name(void);
void comm_telemetry_debug_handle_stream(char *mode, comm_telemetry_debug_write_line_t write_line);
void comm_telemetry_debug_handle_imu(char *sub_command, comm_telemetry_debug_write_line_t write_line);
void comm_telemetry_debug_tick(comm_telemetry_debug_write_line_t write_line);

#endif
