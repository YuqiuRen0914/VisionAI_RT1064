#include "comm_speed_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_error.h"
#include "motion.h"

#define COMM_SPEED_DEBUG_RESPONSE_BUFFER_SIZE (384U)

static void comm_speed_debug_send_line(comm_speed_debug_write_line_t write_line,
                                       const char *format,
                                       ...)
{
    char buffer[COMM_SPEED_DEBUG_RESPONSE_BUFFER_SIZE];
    va_list args;
    int length;

    if(write_line == 0)
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

static ai_status_t comm_speed_debug_parse_i32(const char *text, int32_t *value)
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

static ai_status_t comm_speed_debug_parse_f32(const char *text, float *value)
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

static void comm_speed_debug_handle_arm(comm_speed_debug_write_line_t write_line)
{
    if(motion_speed_bench_arm() == AI_OK)
    {
        comm_speed_debug_send_line(write_line, "OK speed arm");
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed arm");
    }
}

static void comm_speed_debug_handle_wheel(comm_speed_debug_write_line_t write_line)
{
    char *wheel_text = strtok(0, " ");
    char *speed_text = strtok(0, " ");
    int32_t wheel_id;
    float target_mm_s;
    ai_status_t status;

    if((comm_speed_debug_parse_i32(wheel_text, &wheel_id) != AI_OK) ||
       (comm_speed_debug_parse_f32(speed_text, &target_mm_s) != AI_OK))
    {
        comm_speed_debug_send_line(write_line, "ERR speed wheel usage");
        return;
    }

    status = motion_speed_bench_set_wheel((uint8_t)wheel_id, target_mm_s);
    if(status == AI_OK)
    {
        comm_speed_debug_send_line(write_line,
                                   "OK speed wheel=%ld target_mm_s=%.1f",
                                   (long)wheel_id,
                                   (double)target_mm_s);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_speed_debug_send_line(write_line, "ERR speed disarmed, send speed arm");
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed wheel bad_arg");
    }
}

static void comm_speed_debug_handle_all(comm_speed_debug_write_line_t write_line)
{
    char *w1_text = strtok(0, " ");
    char *w2_text = strtok(0, " ");
    char *w3_text = strtok(0, " ");
    char *w4_text = strtok(0, " ");
    float w1;
    float w2;
    float w3;
    float w4;
    ai_status_t status;

    if((comm_speed_debug_parse_f32(w1_text, &w1) != AI_OK) ||
       (comm_speed_debug_parse_f32(w2_text, &w2) != AI_OK) ||
       (comm_speed_debug_parse_f32(w3_text, &w3) != AI_OK) ||
       (comm_speed_debug_parse_f32(w4_text, &w4) != AI_OK))
    {
        comm_speed_debug_send_line(write_line, "ERR speed all usage");
        return;
    }

    status = motion_speed_bench_set_all(w1, w2, w3, w4);
    if(status == AI_OK)
    {
        comm_speed_debug_send_line(write_line,
                                   "OK speed all w1=%.1f w2=%.1f w3=%.1f w4=%.1f",
                                   (double)w1,
                                   (double)w2,
                                   (double)w3,
                                   (double)w4);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_speed_debug_send_line(write_line, "ERR speed disarmed, send speed arm");
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed all bad_arg");
    }
}

static void comm_speed_debug_handle_pid(comm_speed_debug_write_line_t write_line)
{
    char *kp_text = strtok(0, " ");
    char *ki_text = strtok(0, " ");
    char *kd_text = strtok(0, " ");
    float kp;
    float ki;
    float kd;

    if((comm_speed_debug_parse_f32(kp_text, &kp) != AI_OK) ||
       (comm_speed_debug_parse_f32(ki_text, &ki) != AI_OK) ||
       (comm_speed_debug_parse_f32(kd_text, &kd) != AI_OK))
    {
        comm_speed_debug_send_line(write_line, "ERR speed pid usage");
        return;
    }

    if(motion_speed_bench_set_pid(kp, ki, kd) == AI_OK)
    {
        comm_speed_debug_send_line(write_line,
                                   "OK speed pid kp=%.4f ki=%.4f kd=%.4f",
                                   (double)kp,
                                   (double)ki,
                                   (double)kd);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed pid bad_arg");
    }
}

static void comm_speed_debug_handle_limit(comm_speed_debug_write_line_t write_line)
{
    char *duty_text = strtok(0, " ");
    char *speed_text = strtok(0, " ");
    float duty_limit;
    float max_speed;

    if((comm_speed_debug_parse_f32(duty_text, &duty_limit) != AI_OK) ||
       (comm_speed_debug_parse_f32(speed_text, &max_speed) != AI_OK))
    {
        comm_speed_debug_send_line(write_line, "ERR speed limit usage");
        return;
    }

    if(motion_speed_bench_set_limits(duty_limit, max_speed) == AI_OK)
    {
        comm_speed_debug_send_line(write_line,
                                   "OK speed limit duty=%.1f max_mm_s=%.1f",
                                   (double)duty_limit,
                                   (double)max_speed);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed limit bad_arg");
    }
}

static void comm_speed_debug_handle_static(comm_speed_debug_write_line_t write_line)
{
    char *duty_text = strtok(0, " ");
    char *threshold_text = strtok(0, " ");
    float duty_percent;
    float threshold_mm_s;

    if((comm_speed_debug_parse_f32(duty_text, &duty_percent) != AI_OK) ||
       (comm_speed_debug_parse_f32(threshold_text, &threshold_mm_s) != AI_OK))
    {
        comm_speed_debug_send_line(write_line, "ERR speed static usage");
        return;
    }

    if(motion_speed_bench_set_static(duty_percent, threshold_mm_s) == AI_OK)
    {
        comm_speed_debug_send_line(write_line,
                                   "OK speed static duty=%.1f threshold_mm_s=%.1f",
                                   (double)duty_percent,
                                   (double)threshold_mm_s);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed static bad_arg");
    }
}

static void comm_speed_debug_handle_feedforward(comm_speed_debug_write_line_t write_line)
{
    char *ff_text = strtok(0, " ");
    float duty_per_mm_s;

    if(comm_speed_debug_parse_f32(ff_text, &duty_per_mm_s) != AI_OK)
    {
        comm_speed_debug_send_line(write_line, "ERR speed ff usage");
        return;
    }

    if(motion_speed_bench_set_feedforward(duty_per_mm_s) == AI_OK)
    {
        comm_speed_debug_send_line(write_line, "OK speed ff duty_per_mm_s=%.4f", (double)duty_per_mm_s);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed ff bad_arg");
    }
}

static void comm_speed_debug_handle_filter(comm_speed_debug_write_line_t write_line)
{
    char *tau_text = strtok(0, " ");
    float tau_ms;

    if(comm_speed_debug_parse_f32(tau_text, &tau_ms) != AI_OK)
    {
        comm_speed_debug_send_line(write_line, "ERR speed filter usage");
        return;
    }

    if(motion_speed_bench_set_filter(tau_ms) == AI_OK)
    {
        comm_speed_debug_send_line(write_line, "OK speed filter tau_ms=%.1f", (double)tau_ms);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed filter bad_arg");
    }
}

void comm_speed_debug_handle(char *sub_command, comm_speed_debug_write_line_t write_line)
{
    if(sub_command == 0)
    {
        comm_speed_debug_send_line(write_line, "ERR speed usage");
        return;
    }

    if(strcmp(sub_command, "arm") == 0)
    {
        comm_speed_debug_handle_arm(write_line);
    }
    else if(strcmp(sub_command, "stop") == 0)
    {
        (void)motion_speed_bench_stop();
        comm_speed_debug_send_line(write_line, "OK speed stop");
    }
    else if(strcmp(sub_command, "wheel") == 0)
    {
        comm_speed_debug_handle_wheel(write_line);
    }
    else if(strcmp(sub_command, "all") == 0)
    {
        comm_speed_debug_handle_all(write_line);
    }
    else if(strcmp(sub_command, "pid") == 0)
    {
        comm_speed_debug_handle_pid(write_line);
    }
    else if(strcmp(sub_command, "limit") == 0)
    {
        comm_speed_debug_handle_limit(write_line);
    }
    else if(strcmp(sub_command, "static") == 0)
    {
        comm_speed_debug_handle_static(write_line);
    }
    else if(strcmp(sub_command, "ff") == 0)
    {
        comm_speed_debug_handle_feedforward(write_line);
    }
    else if(strcmp(sub_command, "filter") == 0)
    {
        comm_speed_debug_handle_filter(write_line);
    }
    else
    {
        comm_speed_debug_send_line(write_line, "ERR speed usage");
    }
}
