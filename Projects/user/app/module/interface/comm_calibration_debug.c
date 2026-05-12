#include "comm_calibration_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_error.h"
#include "calibration/calibration_telemetry.h"
#include "drive.h"

#define COMM_CALIBRATION_DEBUG_RESPONSE_BUFFER_SIZE (384U)

static calibration_encoder_state_t comm_calibration_debug_encoder;

static void comm_calibration_debug_send_line(comm_calibration_debug_write_line_t write_line,
                                             const char *format,
                                             ...)
{
    char buffer[COMM_CALIBRATION_DEBUG_RESPONSE_BUFFER_SIZE];
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

static ai_status_t comm_calibration_debug_parse_i32(const char *text, int32_t *value)
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

static void comm_calibration_debug_encoder_total_to_counts(const DriveEncoderTotal *total,
                                                           calibration_wheel_counts_t *counts)
{
    counts->wheel1 = total->wheel1;
    counts->wheel2 = total->wheel2;
    counts->wheel3 = total->wheel3;
    counts->wheel4 = total->wheel4;
}

static ai_status_t comm_calibration_debug_read_encoder_counts(calibration_wheel_counts_t *counts)
{
    DriveEncoderTotal total;

    if(counts == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(DriveGetAllEncoderTotal(&total) != DRIVE_STATUS_OK)
    {
        return AI_ERR_NO_DATA;
    }

    comm_calibration_debug_encoder_total_to_counts(&total, counts);
    return AI_OK;
}

static void comm_calibration_debug_send_encoder_counts(comm_calibration_debug_write_line_t write_line,
                                                       const char *label,
                                                       const calibration_wheel_counts_t *counts)
{
    comm_calibration_debug_send_line(write_line,
                                     "DATA %s w1=%ld w2=%ld w3=%ld w4=%ld",
                                     label,
                                     (long)counts->wheel1,
                                     (long)counts->wheel2,
                                     (long)counts->wheel3,
                                     (long)counts->wheel4);
}

static void comm_calibration_debug_handle_enc(char *enc_command,
                                              comm_calibration_debug_write_line_t write_line)
{
    calibration_wheel_counts_t counts;
    calibration_wheel_counts_t delta;
    calibration_encoder_turn_result_t result;
    char *wheel_text;
    char *turns_text;
    int32_t wheel_id;
    int32_t turns;

    if(enc_command == 0)
    {
        comm_calibration_debug_send_line(write_line, "ERR cal enc usage");
        return;
    }

    if(strcmp(enc_command, "zero") == 0)
    {
        if(comm_calibration_debug_read_encoder_counts(&counts) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc no_data");
            return;
        }

        calibration_encoder_reset_baseline(&comm_calibration_debug_encoder, &counts);
        comm_calibration_debug_send_line(write_line,
                                         "OK cal enc zero w1=%ld w2=%ld w3=%ld w4=%ld",
                                         (long)counts.wheel1,
                                         (long)counts.wheel2,
                                         (long)counts.wheel3,
                                         (long)counts.wheel4);
    }
    else if(strcmp(enc_command, "total") == 0)
    {
        if(comm_calibration_debug_read_encoder_counts(&counts) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc no_data");
            return;
        }

        comm_calibration_debug_send_encoder_counts(write_line, "cal_enc_total", &counts);
    }
    else if(strcmp(enc_command, "delta") == 0)
    {
        if(comm_calibration_debug_read_encoder_counts(&counts) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc no_data");
            return;
        }

        if(calibration_encoder_delta(&comm_calibration_debug_encoder, &counts, &delta) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc no_data");
            return;
        }

        comm_calibration_debug_send_encoder_counts(write_line, "cal_enc_delta", &delta);
    }
    else if(strcmp(enc_command, "wheel") == 0)
    {
        wheel_text = strtok(0, " ");
        turns_text = strtok(0, " ");

        if(comm_calibration_debug_parse_i32(wheel_text, &wheel_id) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc bad_wheel");
            return;
        }

        if((wheel_id < 1) || (wheel_id > 4))
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc bad_wheel");
            return;
        }

        if((comm_calibration_debug_parse_i32(turns_text, &turns) != AI_OK) || (turns <= 0))
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc bad_turns");
            return;
        }

        if(comm_calibration_debug_read_encoder_counts(&counts) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc no_data");
            return;
        }

        if(calibration_encoder_counts_per_rev(&comm_calibration_debug_encoder,
                                              &counts,
                                              (uint8_t)wheel_id,
                                              (uint32_t)turns,
                                              &result) != AI_OK)
        {
            comm_calibration_debug_send_line(write_line, "ERR cal enc bad_arg");
            return;
        }

        comm_calibration_debug_send_line(write_line,
                                         "OK cal enc wheel=%ld counts=%ld turns=%lu counts_per_rev_x100=%ld",
                                         (long)wheel_id,
                                         (long)result.counts,
                                         (unsigned long)result.turns,
                                         (long)result.counts_per_rev_x100);
    }
    else
    {
        comm_calibration_debug_send_line(write_line, "ERR cal enc usage");
    }
}

void comm_calibration_debug_init(void)
{
    DriveEncoderTotal total;
    calibration_wheel_counts_t counts;

    if(DriveGetAllEncoderTotal(&total) != DRIVE_STATUS_OK)
    {
        total.wheel1 = 0;
        total.wheel2 = 0;
        total.wheel3 = 0;
        total.wheel4 = 0;
    }

    comm_calibration_debug_encoder_total_to_counts(&total, &counts);
    calibration_encoder_reset_baseline(&comm_calibration_debug_encoder, &counts);
}

void comm_calibration_debug_handle(char *sub_command, comm_calibration_debug_write_line_t write_line)
{
    if(sub_command == 0)
    {
        comm_calibration_debug_send_line(write_line, "ERR cal usage");
        return;
    }

    if(strcmp(sub_command, "enc") == 0)
    {
        comm_calibration_debug_handle_enc(strtok(0, " "), write_line);
    }
    else
    {
        comm_calibration_debug_send_line(write_line, "ERR cal usage");
    }
}
