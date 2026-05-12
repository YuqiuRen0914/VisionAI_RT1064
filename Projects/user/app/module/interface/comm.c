#include "comm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ai_config.h"
#include "comm_calibration_debug.h"
#include "comm_motion_debug.h"
#include "comm_speed_debug.h"
#include "comm_telemetry_debug.h"
#include "comm_vision_debug.h"
#include "motion.h"
#include "wireless.h"
#include "zf_common_debug.h"

#define COMM_LINE_BUFFER_SIZE        (96U)
#define COMM_RX_BUFFER_SIZE          (32U)
#define COMM_RESPONSE_BUFFER_SIZE    (384U)
#define COMM_COMMAND_IDLE_MS         (200U)
#define COMM_READY_REPORT_PERIOD_MS  (1000U)

static char comm_line_buffer[COMM_LINE_BUFFER_SIZE];
static uint32_t comm_line_length;
static uint32_t comm_line_idle_ms;
static uint8_t comm_line_overflow;
static uint8_t comm_command_seen;
static uint32_t comm_ready_report_elapsed_ms;
static uint32_t comm_ready_report_count;

static void comm_send_raw(const char *text)
{
    (void)debug_send_buffer((const uint8 *)text, (uint32)strlen(text));
}

static void comm_send_line(const char *format, ...)
{
    char buffer[COMM_RESPONSE_BUFFER_SIZE];
    va_list args;
    int length;

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
    }

    (void)debug_send_buffer((const uint8 *)buffer, (uint32)length);
    comm_send_raw("\r\n");
}

static void comm_write_line_text(const char *text)
{
    comm_send_line("%s", text);
}

static const char *comm_motion_mode_name(motion_mode_t mode)
{
    if(mode == MOTION_MODE_OPEN_LOOP_TEST)
    {
        return "open_loop";
    }

    if(mode == MOTION_MODE_SPEED_BENCH)
    {
        return "speed_bench";
    }

    return "action_closed_loop";
}

static void comm_send_help(void)
{
    comm_send_line("OK help");
    comm_send_line("help");
    comm_send_line("stop");
    comm_send_line("arm");
    comm_send_line("disarm");
    comm_send_line("status");
    comm_send_line("imu zero");
    comm_send_line("imu yawx zero");
    comm_send_line("imu stat <ms>");
    comm_send_line("stream off|imu_acc|imu_gyro|imu_yawx|enc5|enc100|duty|speed");
    comm_send_line("stream data is text: DATA <mode> ...");
    comm_send_line("cal enc zero");
    comm_send_line("cal enc total|delta");
    comm_send_line("cal enc wheel <1|2|3|4> <turns>");
    comm_send_line("speed arm|stop");
    comm_send_line("speed wheel <1|2|3|4> <mm_s>");
    comm_send_line("speed all <w1_mm_s> <w2_mm_s> <w3_mm_s> <w4_mm_s>");
    comm_send_line("speed pid <kp> <ki> <kd>");
    comm_send_line("speed limit <duty_pct> <max_mm_s>");
    comm_send_line("speed static <duty_pct> <measured_threshold_mm_s>");
    comm_send_line("speed ff <duty_per_mm_s>");
    comm_send_line("speed filter <tau_ms>");
    comm_send_line("motor <1|2|3|4|all> <duty_pct> <ms>");
    comm_send_line("move <fwd|back|left|right|ccw|cw> <duty_pct> <ms>");
    comm_send_line("vision");
    comm_send_line("vision clear");
    comm_send_line("vision sim move <up|down|left|right> <cm> [seq]");
    comm_send_line("vision sim rotate <ccw|cw> <90|180> [seq]");
    comm_send_line("vision sim stop|query|reset [seq]");
    comm_send_line("vision sim raw <seq> <cmd> <dir> <val>");
}

static void comm_handle_line(char *line)
{
    char *command = strtok(line, " ");
    motion_action_debug_t action_debug;

    if(command == NULL)
    {
        return;
    }

    if(strcmp(command, "help") == 0)
    {
        comm_send_help();
    }
    else if(strcmp(command, "stop") == 0)
    {
        motion_action_stop_all();
        comm_send_line("OK stop");
    }
    else if(strcmp(command, "arm") == 0)
    {
        comm_motion_debug_handle_arm(comm_write_line_text);
    }
    else if(strcmp(command, "disarm") == 0)
    {
        comm_motion_debug_handle_disarm(comm_write_line_text);
    }
    else if(strcmp(command, "status") == 0)
    {
        comm_send_line("OK status mode=%s armed=%u speed_armed=%u stream=%s",
                       comm_motion_mode_name(motion_get_mode()),
                       (unsigned int)motion_test_is_armed(),
                       (unsigned int)motion_speed_bench_is_armed(),
                       comm_telemetry_debug_stream_name());
        motion_get_action_debug(&action_debug);
        comm_send_line("DATA action cmd=%u dir=%u val=%u elapsed=%lu stable=%lu blocked=%lu vx=%.1f vy=%.1f rot=%.1f",
                       (unsigned int)action_debug.cmd,
                       (unsigned int)action_debug.dir,
                       (unsigned int)action_debug.val,
                       (unsigned long)action_debug.elapsed_ms,
                       (unsigned long)action_debug.completion_elapsed_ms,
                       (unsigned long)action_debug.blocked_ms,
                       (double)action_debug.vx_mm_s,
                       (double)action_debug.vy_mm_s,
                       (double)action_debug.rot_mm_s);
        comm_send_line("DATA action_feedback target_mm=%.1f x=%.1f y=%.1f axis_err=%.1f target_deg=%.1f heading=%.1f heading_err=%.1f imu_rate=%.1f",
                       (double)action_debug.target_distance_mm,
                       (double)action_debug.x_mm,
                       (double)action_debug.y_mm,
                       (double)action_debug.axis_error_mm,
                       (double)action_debug.target_angle_deg,
                       (double)action_debug.heading_deg,
                       (double)action_debug.heading_error_deg,
                       (double)action_debug.imu_rate_dps);
    }
    else if(strcmp(command, "stream") == 0)
    {
        comm_telemetry_debug_handle_stream(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "imu") == 0)
    {
        comm_telemetry_debug_handle_imu(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "cal") == 0)
    {
        comm_calibration_debug_handle(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "speed") == 0)
    {
        comm_speed_debug_handle(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "motor") == 0)
    {
        comm_motion_debug_handle_motor(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "move") == 0)
    {
        comm_motion_debug_handle_move(strtok(NULL, " "), comm_write_line_text);
    }
    else if(strcmp(command, "vision") == 0)
    {
        comm_vision_debug_handle(strtok(NULL, " "), comm_write_line_text);
    }
    else
    {
        comm_send_line("ERR unknown_command=%s", command);
    }
}

static void comm_finish_line(void)
{
    if(comm_line_overflow != 0U)
    {
        comm_line_length = 0;
        comm_line_idle_ms = 0;
        comm_line_overflow = 0;
        comm_send_line("ERR line_too_long");
        return;
    }

    if(comm_line_length == 0U)
    {
        comm_line_idle_ms = 0;
        return;
    }

    comm_line_buffer[comm_line_length] = '\0';
    comm_command_seen = 1U;
    comm_send_line("OK rx %s", comm_line_buffer);
    comm_handle_line(comm_line_buffer);
    comm_line_length = 0;
    comm_line_idle_ms = 0;
}

static void comm_handle_rx_byte(char byte)
{
    if((byte == '\r') || (byte == '\n'))
    {
        comm_finish_line();
        return;
    }

    if(comm_line_overflow != 0U)
    {
        return;
    }

    if(comm_line_length >= (COMM_LINE_BUFFER_SIZE - 1U))
    {
        comm_line_overflow = 1U;
        return;
    }

    comm_line_buffer[comm_line_length] = byte;
    comm_line_length++;
    comm_line_idle_ms = 0;
}

static void comm_poll_rx(void)
{
    uint8 buffer[COMM_RX_BUFFER_SIZE];
    uint32 length;

    length = debug_read_ring_buffer(buffer, sizeof(buffer));
    for(uint32 i = 0; i < length; i++)
    {
        comm_handle_rx_byte((char)buffer[i]);
    }
}

static void comm_update_idle_line(void)
{
    if((comm_line_length == 0U) || (comm_line_overflow != 0U))
    {
        return;
    }

    comm_line_idle_ms += AI_COMM_PERIOD_MS;
    if(comm_line_idle_ms >= COMM_COMMAND_IDLE_MS)
    {
        comm_finish_line();
    }
}

static void comm_update_ready_report(void)
{
    if(comm_command_seen != 0U)
    {
        return;
    }

    comm_ready_report_elapsed_ms += AI_COMM_PERIOD_MS;
    if(comm_ready_report_elapsed_ms < COMM_READY_REPORT_PERIOD_MS)
    {
        return;
    }

    comm_ready_report_elapsed_ms = 0;
    comm_ready_report_count++;
    comm_send_line("OK closed_loop ready uart=UART1 baud=115200 tick=%lu", (unsigned long)comm_ready_report_count);
}

ai_status_t comm_module_init(void)
{
    comm_line_length = 0;
    comm_line_idle_ms = 0;
    comm_line_overflow = 0;
    comm_command_seen = 0;
    comm_ready_report_elapsed_ms = 0;
    comm_ready_report_count = 0;

    comm_calibration_debug_init();
    comm_telemetry_debug_init();

    comm_send_line("OK closed_loop ready uart=UART1 baud=115200, type help");

    return WirelessInit();
}

void comm_module_tick(void)
{
    comm_poll_rx();
    comm_update_idle_line();
    comm_update_ready_report();
    comm_telemetry_debug_tick(comm_write_line_text);
}
