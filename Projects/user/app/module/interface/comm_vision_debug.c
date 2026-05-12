#include "comm_vision_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_error.h"
#include "vision.h"
#include "vision_protocol.h"

#define COMM_VISION_DEBUG_RESPONSE_BUFFER_SIZE (384U)

static uint8_t comm_vision_debug_sim_seq;

static void comm_vision_debug_send_line(comm_vision_debug_write_line_t write_line,
                                        const char *format,
                                        ...)
{
    char buffer[COMM_VISION_DEBUG_RESPONSE_BUFFER_SIZE];
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

static ai_status_t comm_vision_debug_parse_i32(const char *text, int32_t *value)
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

static ai_status_t comm_vision_debug_parse_u8_auto(const char *text, uint8_t *value)
{
    char *end_ptr;
    unsigned long parsed;

    if((text == 0) || (value == 0) || (text[0] == '\0'))
    {
        return AI_ERR_INVALID_ARG;
    }

    parsed = strtoul(text, &end_ptr, 0);
    if((end_ptr == text) || (*end_ptr != '\0') || (parsed > UINT8_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    *value = (uint8_t)parsed;
    return AI_OK;
}

static void comm_vision_debug_send_sim_result(comm_vision_debug_write_line_t write_line,
                                              ai_status_t status,
                                              const uint8_t *frame,
                                              uint8_t explicit_seq)
{
    if(status == AI_OK)
    {
        if(explicit_seq == 0U)
        {
            comm_vision_debug_sim_seq++;
        }

        comm_vision_debug_send_line(write_line,
                                    "OK vision sim %02X %02X %02X %02X %02X %02X %02X",
                                    (unsigned int)frame[0],
                                    (unsigned int)frame[1],
                                    (unsigned int)frame[2],
                                    (unsigned int)frame[3],
                                    (unsigned int)frame[4],
                                    (unsigned int)frame[5],
                                    (unsigned int)frame[6]);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim busy");
    }
    else
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim bad_arg");
    }
}

static void comm_vision_debug_inject_sim_frame(comm_vision_debug_write_line_t write_line,
                                               uint8_t seq,
                                               uint8_t cmd,
                                               uint8_t dir,
                                               uint8_t val,
                                               uint8_t explicit_seq)
{
    uint8_t frame[VISION_PROTOCOL_FRAME_LENGTH];
    ai_status_t status;

    status = vision_protocol_inject_frame(seq, cmd, dir, val, frame);
    comm_vision_debug_send_sim_result(write_line, status, frame, explicit_seq);
}

static ai_status_t comm_vision_debug_get_sim_seq(char *seq_text, uint8_t *seq, uint8_t *explicit_seq)
{
    if((seq == 0) || (explicit_seq == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(seq_text == 0)
    {
        *seq = comm_vision_debug_sim_seq;
        *explicit_seq = 0U;
        return AI_OK;
    }

    if(comm_vision_debug_parse_u8_auto(seq_text, seq) != AI_OK)
    {
        return AI_ERR_INVALID_ARG;
    }

    *explicit_seq = 1U;
    return AI_OK;
}

static ai_status_t comm_vision_debug_parse_move_dir(const char *dir_text, uint8_t *dir)
{
    if((dir_text == 0) || (dir == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(strcmp(dir_text, "up") == 0)
    {
        *dir = VISION_PROTOCOL_MOVE_UP;
    }
    else if(strcmp(dir_text, "down") == 0)
    {
        *dir = VISION_PROTOCOL_MOVE_DOWN;
    }
    else if(strcmp(dir_text, "left") == 0)
    {
        *dir = VISION_PROTOCOL_MOVE_LEFT;
    }
    else if(strcmp(dir_text, "right") == 0)
    {
        *dir = VISION_PROTOCOL_MOVE_RIGHT;
    }
    else
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_vision_debug_parse_rotate_dir(const char *dir_text, uint8_t *dir)
{
    if((dir_text == 0) || (dir == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(strcmp(dir_text, "ccw") == 0)
    {
        *dir = VISION_PROTOCOL_ROTATE_CCW;
    }
    else if(strcmp(dir_text, "cw") == 0)
    {
        *dir = VISION_PROTOCOL_ROTATE_CW;
    }
    else
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_vision_debug_parse_angle(const char *angle_text, uint8_t *val)
{
    int32_t angle;

    if((val == 0) || (comm_vision_debug_parse_i32(angle_text, &angle) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(angle == 90)
    {
        *val = VISION_PROTOCOL_ROTATE_90_VAL;
    }
    else if(angle == 180)
    {
        *val = VISION_PROTOCOL_ROTATE_180_VAL;
    }
    else
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static void comm_vision_debug_handle_sim_move(comm_vision_debug_write_line_t write_line)
{
    char *dir_text = strtok(0, " ");
    char *cm_text = strtok(0, " ");
    char *seq_text = strtok(0, " ");
    uint8_t seq;
    uint8_t explicit_seq;
    uint8_t dir;
    int32_t cm;

    if((dir_text == 0) ||
       (cm_text == 0) ||
       (comm_vision_debug_parse_move_dir(dir_text, &dir) != AI_OK) ||
       (comm_vision_debug_parse_i32(cm_text, &cm) != AI_OK) ||
       (cm <= 0) ||
       (cm > UINT8_MAX) ||
       (comm_vision_debug_get_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK))
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim bad_arg");
        return;
    }

    comm_vision_debug_inject_sim_frame(write_line, seq, VISION_PROTOCOL_CMD_MOVE, dir, (uint8_t)cm, explicit_seq);
}

static void comm_vision_debug_handle_sim_rotate(comm_vision_debug_write_line_t write_line)
{
    char *dir_text = strtok(0, " ");
    char *angle_text = strtok(0, " ");
    char *seq_text = strtok(0, " ");
    uint8_t seq;
    uint8_t explicit_seq;
    uint8_t dir;
    uint8_t val;

    if((dir_text == 0) ||
       (angle_text == 0) ||
       (comm_vision_debug_parse_rotate_dir(dir_text, &dir) != AI_OK) ||
       (comm_vision_debug_parse_angle(angle_text, &val) != AI_OK) ||
       (comm_vision_debug_get_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK))
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim bad_arg");
        return;
    }

    comm_vision_debug_inject_sim_frame(write_line, seq, VISION_PROTOCOL_CMD_ROTATE, dir, val, explicit_seq);
}

static void comm_vision_debug_handle_sim_simple(comm_vision_debug_write_line_t write_line, uint8_t cmd)
{
    char *seq_text = strtok(0, " ");
    uint8_t seq;
    uint8_t explicit_seq;

    if(comm_vision_debug_get_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK)
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim bad_arg");
        return;
    }

    comm_vision_debug_inject_sim_frame(write_line, seq, cmd, 0U, 0U, explicit_seq);
}

static void comm_vision_debug_handle_sim_raw(comm_vision_debug_write_line_t write_line)
{
    char *seq_text = strtok(0, " ");
    char *cmd_text = strtok(0, " ");
    char *dir_text = strtok(0, " ");
    char *val_text = strtok(0, " ");
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;

    if((comm_vision_debug_parse_u8_auto(seq_text, &seq) != AI_OK) ||
       (comm_vision_debug_parse_u8_auto(cmd_text, &cmd) != AI_OK) ||
       (comm_vision_debug_parse_u8_auto(dir_text, &dir) != AI_OK) ||
       (comm_vision_debug_parse_u8_auto(val_text, &val) != AI_OK))
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim bad_arg");
        return;
    }

    comm_vision_debug_inject_sim_frame(write_line, seq, cmd, dir, val, 1U);
}

static void comm_vision_debug_handle_sim_command(comm_vision_debug_write_line_t write_line)
{
    char *sim_command = strtok(0, " ");

    if(sim_command == 0)
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim usage");
        return;
    }

    if(strcmp(sim_command, "move") == 0)
    {
        comm_vision_debug_handle_sim_move(write_line);
    }
    else if(strcmp(sim_command, "rotate") == 0)
    {
        comm_vision_debug_handle_sim_rotate(write_line);
    }
    else if(strcmp(sim_command, "stop") == 0)
    {
        comm_vision_debug_handle_sim_simple(write_line, VISION_PROTOCOL_CMD_STOP);
    }
    else if(strcmp(sim_command, "query") == 0)
    {
        comm_vision_debug_handle_sim_simple(write_line, VISION_PROTOCOL_CMD_QUERY);
    }
    else if(strcmp(sim_command, "reset") == 0)
    {
        comm_vision_debug_handle_sim_simple(write_line, VISION_PROTOCOL_CMD_RESET);
    }
    else if(strcmp(sim_command, "raw") == 0)
    {
        comm_vision_debug_handle_sim_raw(write_line);
    }
    else
    {
        comm_vision_debug_send_line(write_line, "ERR vision sim usage");
    }
}

static void comm_vision_debug_send_status(comm_vision_debug_write_line_t write_line)
{
    vision_debug_t debug;

    vision_debug_get(&debug);
    comm_vision_debug_send_line(write_line,
                                "OK vision status=%u parser=%u active_seq=%u rx_bytes=%lu frames_ok=%lu bad_frame=%lu bad_cmd=%lu busy=%lu motion_err=%lu",
                                (unsigned int)debug.status,
                                (unsigned int)debug.parser_state,
                                (unsigned int)debug.active_seq,
                                (unsigned long)debug.rx_bytes,
                                (unsigned long)debug.frames_ok,
                                (unsigned long)debug.bad_frames,
                                (unsigned long)debug.bad_cmds,
                                (unsigned long)debug.busy_errors,
                                (unsigned long)debug.motion_errors);
    comm_vision_debug_send_line(write_line,
                                "OK vision cmd move=%lu rotate=%lu stop=%lu query=%lu reset=%lu last=%02X %02X %02X %02X err=%02X:%02X:%02X ovf=%lu",
                                (unsigned long)debug.move_cmds,
                                (unsigned long)debug.rotate_cmds,
                                (unsigned long)debug.stop_cmds,
                                (unsigned long)debug.query_cmds,
                                (unsigned long)debug.reset_cmds,
                                (unsigned int)debug.last_seq,
                                (unsigned int)debug.last_cmd,
                                (unsigned int)debug.last_dir,
                                (unsigned int)debug.last_val,
                                (unsigned int)debug.last_error_seq,
                                (unsigned int)debug.last_error,
                                (unsigned int)debug.last_error_context,
                                (unsigned long)debug.rx_overflows);
    comm_vision_debug_send_line(write_line,
                                "DATA vision_rx count=%u %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                                (unsigned int)debug.last_rx_count,
                                (unsigned int)debug.last_rx[0],
                                (unsigned int)debug.last_rx[1],
                                (unsigned int)debug.last_rx[2],
                                (unsigned int)debug.last_rx[3],
                                (unsigned int)debug.last_rx[4],
                                (unsigned int)debug.last_rx[5],
                                (unsigned int)debug.last_rx[6],
                                (unsigned int)debug.last_rx[7],
                                (unsigned int)debug.last_rx[8],
                                (unsigned int)debug.last_rx[9],
                                (unsigned int)debug.last_rx[10],
                                (unsigned int)debug.last_rx[11],
                                (unsigned int)debug.last_rx[12],
                                (unsigned int)debug.last_rx[13],
                                (unsigned int)debug.last_rx[14],
                                (unsigned int)debug.last_rx[15]);
}

void comm_vision_debug_handle(char *sub_command, comm_vision_debug_write_line_t write_line)
{
    if((sub_command != 0) && (strcmp(sub_command, "sim") == 0))
    {
        comm_vision_debug_handle_sim_command(write_line);
        return;
    }

    if((sub_command != 0) && (strcmp(sub_command, "clear") == 0))
    {
        vision_debug_clear();
        comm_vision_debug_send_line(write_line, "OK vision clear");
        return;
    }

    comm_vision_debug_send_status(write_line);
}
