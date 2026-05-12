#include "comm_motion_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_config.h"
#include "ai_error.h"
#include "motion.h"

#define COMM_MOTION_DEBUG_RESPONSE_BUFFER_SIZE (384U)

static void comm_motion_debug_send_line(comm_motion_debug_write_line_t write_line,
                                        const char *format,
                                        ...)
{
    char buffer[COMM_MOTION_DEBUG_RESPONSE_BUFFER_SIZE];
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

static ai_status_t comm_motion_debug_parse_i32(const char *text, int32_t *value)
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

static ai_status_t comm_motion_debug_validate_duty_arg(int32_t duty)
{
    if((duty > AI_TEST_DUTY_PERCENT_MAX) || (duty < -AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_motion_debug_validate_run_ms_arg(int32_t run_ms)
{
    if((run_ms < 0) || ((uint32_t)run_ms > AI_TEST_RUN_MS_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_motion_debug_validate_move_duty_arg(int32_t duty)
{
    if((duty < 0) || (duty > AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

void comm_motion_debug_handle_arm(comm_motion_debug_write_line_t write_line)
{
    (void)motion_test_arm();
    comm_motion_debug_send_line(write_line, "OK arm");
}

void comm_motion_debug_handle_disarm(comm_motion_debug_write_line_t write_line)
{
    (void)motion_test_disarm();
    comm_motion_debug_send_line(write_line, "OK disarm");
}

void comm_motion_debug_handle_motor(char *wheel_text, comm_motion_debug_write_line_t write_line)
{
    char *duty_text = strtok(0, " ");
    char *ms_text = strtok(0, " ");
    int32_t duty;
    int32_t run_ms;
    int32_t wheel_id;
    ai_status_t status;

    if((wheel_text == 0) ||
       (comm_motion_debug_parse_i32(duty_text, &duty) != AI_OK) ||
       (comm_motion_debug_parse_i32(ms_text, &run_ms) != AI_OK) ||
       (comm_motion_debug_validate_duty_arg(duty) != AI_OK) ||
       (comm_motion_debug_validate_run_ms_arg(run_ms) != AI_OK))
    {
        comm_motion_debug_send_line(write_line, "ERR motor usage");
        return;
    }

    if(strcmp(wheel_text, "all") == 0)
    {
        status = motion_test_set_all((int16_t)duty,
                                     (int16_t)duty,
                                     (int16_t)duty,
                                     (int16_t)duty,
                                     (uint32_t)run_ms);
    }
    else if(comm_motion_debug_parse_i32(wheel_text, &wheel_id) == AI_OK)
    {
        status = motion_test_set_motor((uint8_t)wheel_id, (int16_t)duty, (uint32_t)run_ms);
    }
    else
    {
        status = AI_ERR_INVALID_ARG;
    }

    if(status == AI_OK)
    {
        comm_motion_debug_send_line(write_line,
                                    "OK motor %s duty=%ld ms=%ld",
                                    wheel_text,
                                    (long)duty,
                                    (long)run_ms);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_motion_debug_send_line(write_line, "ERR motor disarmed, send arm");
    }
    else
    {
        comm_motion_debug_send_line(write_line, "ERR motor invalid_arg");
    }
}

void comm_motion_debug_handle_move(char *move_text, comm_motion_debug_write_line_t write_line)
{
    char *duty_text = strtok(0, " ");
    char *ms_text = strtok(0, " ");
    int32_t duty;
    int32_t run_ms;
    mecanum_velocity_t velocity;

    if((move_text == 0) ||
       (comm_motion_debug_parse_i32(duty_text, &duty) != AI_OK) ||
       (comm_motion_debug_parse_i32(ms_text, &run_ms) != AI_OK) ||
       (comm_motion_debug_validate_move_duty_arg(duty) != AI_OK) ||
       (comm_motion_debug_validate_run_ms_arg(run_ms) != AI_OK))
    {
        comm_motion_debug_send_line(write_line, "ERR move usage");
        return;
    }

    velocity.vx = 0;
    velocity.vy = 0;
    velocity.wz = 0;

    if(strcmp(move_text, "fwd") == 0)
    {
        velocity.vx = (int16_t)duty;
    }
    else if(strcmp(move_text, "back") == 0)
    {
        velocity.vx = (int16_t)-duty;
    }
    else if(strcmp(move_text, "left") == 0)
    {
        velocity.vy = (int16_t)duty;
    }
    else if(strcmp(move_text, "right") == 0)
    {
        velocity.vy = (int16_t)-duty;
    }
    else if(strcmp(move_text, "ccw") == 0)
    {
        velocity.wz = (int16_t)duty;
    }
    else if(strcmp(move_text, "cw") == 0)
    {
        velocity.wz = (int16_t)-duty;
    }
    else
    {
        comm_motion_debug_send_line(write_line, "ERR move bad_mode=%s", move_text);
        return;
    }

    if(motion_test_set_mecanum(&velocity, (uint32_t)run_ms) == AI_OK)
    {
        comm_motion_debug_send_line(write_line,
                                    "OK move %s duty=%ld ms=%ld",
                                    move_text,
                                    (long)duty,
                                    (long)run_ms);
    }
    else if(motion_test_is_armed() == 0U)
    {
        comm_motion_debug_send_line(write_line, "ERR move disarmed, send arm");
    }
    else
    {
        comm_motion_debug_send_line(write_line, "ERR move invalid_arg");
    }
}
