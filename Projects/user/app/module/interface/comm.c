#include "comm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_config.h"
#include "drive.h"
#include "imu660ra.h"
#include "motion.h"
#include "wireless.h"
#include "zf_common_debug.h"

#define COMM_LINE_BUFFER_SIZE        (96U)
#define COMM_RX_BUFFER_SIZE          (32U)
#define COMM_RESPONSE_BUFFER_SIZE    (192U)
#define COMM_IMU_STAT_MS_MAX         (60000U)
#define COMM_COMMAND_IDLE_MS         (200U)
#define COMM_READY_REPORT_PERIOD_MS  (1000U)

typedef enum
{
    COMM_STREAM_OFF = 0,
    COMM_STREAM_IMU_ACC,
    COMM_STREAM_IMU_GYRO,
    COMM_STREAM_ENC5,
    COMM_STREAM_ENC100,
    COMM_STREAM_DUTY,
} comm_stream_mode_t;

typedef struct
{
    uint8_t active;
    uint32_t remaining_ms;
    uint32_t samples;
    int64_t acc_norm_mg_sum;
    int64_t gyro_x_x10_sum;
    int64_t gyro_y_x10_sum;
    int64_t gyro_z_x10_sum;
    int32_t yaw_start_x10;
    int32_t yaw_last_x10;
} comm_imu_stat_t;

static char comm_line_buffer[COMM_LINE_BUFFER_SIZE];
static uint32_t comm_line_length;
static uint32_t comm_line_idle_ms;
static uint8_t comm_line_overflow;
static uint8_t comm_command_seen;
static uint32_t comm_ready_report_elapsed_ms;
static uint32_t comm_ready_report_count;
static comm_stream_mode_t comm_stream_mode;
static DriveEncoderTotal comm_enc100_last_total;
static uint32_t comm_enc100_elapsed_ms;
static DriveEncoderDelta comm_enc100_delta;
static comm_imu_stat_t comm_imu_stat;

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

static int32_t comm_float_to_i32(float value)
{
    if(value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}

static int16_t comm_saturate_i16(int32_t value)
{
    if(value > INT16_MAX)
    {
        return INT16_MAX;
    }

    if(value < INT16_MIN)
    {
        return INT16_MIN;
    }

    return (int16_t)value;
}

static uint32_t comm_isqrt_u64(uint64_t value)
{
    uint64_t bit = (uint64_t)1 << 62;
    uint64_t result = 0;

    while(bit > value)
    {
        bit >>= 2;
    }

    while(bit != 0U)
    {
        if(value >= (result + bit))
        {
            value -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }

        bit >>= 2;
    }

    return (uint32_t)result;
}

static int32_t comm_acc_norm_mg(int32_t acc_x_mg, int32_t acc_y_mg, int32_t acc_z_mg)
{
    uint64_t sum_square = 0;

    sum_square += (uint64_t)((int64_t)acc_x_mg * acc_x_mg);
    sum_square += (uint64_t)((int64_t)acc_y_mg * acc_y_mg);
    sum_square += (uint64_t)((int64_t)acc_z_mg * acc_z_mg);

    return (int32_t)comm_isqrt_u64(sum_square);
}

static ai_status_t comm_parse_i32(const char *text, int32_t *value)
{
    char *end_ptr;
    long parsed;

    if((text == NULL) || (value == NULL) || (text[0] == '\0'))
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

static ai_status_t comm_validate_duty_arg(int32_t duty)
{
    if((duty > AI_TEST_DUTY_PERCENT_MAX) || (duty < -AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_validate_run_ms_arg(int32_t run_ms)
{
    if((run_ms < 0) || ((uint32_t)run_ms > AI_TEST_RUN_MS_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_validate_move_duty_arg(int32_t duty)
{
    if((duty < 0) || (duty > AI_TEST_DUTY_PERCENT_MAX))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static const char *comm_stream_name(comm_stream_mode_t mode)
{
    if(mode == COMM_STREAM_IMU_ACC)
    {
        return "imu_acc";
    }

    if(mode == COMM_STREAM_IMU_GYRO)
    {
        return "imu_gyro";
    }

    if(mode == COMM_STREAM_ENC5)
    {
        return "enc5";
    }

    if(mode == COMM_STREAM_ENC100)
    {
        return "enc100";
    }

    if(mode == COMM_STREAM_DUTY)
    {
        return "duty";
    }

    return "off";
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
    comm_send_line("imu stat <ms>");
    comm_send_line("stream off|imu_acc|imu_gyro|enc5|enc100|duty");
    comm_send_line("stream data is text: DATA <mode> ...");
    comm_send_line("motor <1|2|3|4|all> <duty_pct> <ms>");
    comm_send_line("move <fwd|back|left|right|ccw|cw> <duty_pct> <ms>");
}

static ai_status_t comm_read_imu_scaled(Imu660raScaledData *imu,
                                        int32_t *acc_x_mg,
                                        int32_t *acc_y_mg,
                                        int32_t *acc_z_mg,
                                        int32_t *gyro_x_x10,
                                        int32_t *gyro_y_x10,
                                        int32_t *gyro_z_x10,
                                        int32_t *yaw_x10)
{
    if(Imu660raGetScaled(imu) != AI_OK)
    {
        return AI_ERR_NO_DATA;
    }

    if(acc_x_mg != NULL)
    {
        *acc_x_mg = comm_float_to_i32(imu->accXG * 1000.0f);
    }

    if(acc_y_mg != NULL)
    {
        *acc_y_mg = comm_float_to_i32(imu->accYG * 1000.0f);
    }

    if(acc_z_mg != NULL)
    {
        *acc_z_mg = comm_float_to_i32(imu->accZG * 1000.0f);
    }

    if(gyro_x_x10 != NULL)
    {
        *gyro_x_x10 = comm_float_to_i32(imu->gyroXDps * 10.0f);
    }

    if(gyro_y_x10 != NULL)
    {
        *gyro_y_x10 = comm_float_to_i32(imu->gyroYDps * 10.0f);
    }

    if(gyro_z_x10 != NULL)
    {
        *gyro_z_x10 = comm_float_to_i32(imu->gyroZDps * 10.0f);
    }

    if(yaw_x10 != NULL)
    {
        *yaw_x10 = comm_float_to_i32(imu->yawDeg * 10.0f);
    }

    return AI_OK;
}

static void comm_start_imu_stat(uint32_t duration_ms)
{
    Imu660raScaledData imu;
    int32_t yaw_x10;

    if(duration_ms == 0U)
    {
        duration_ms = 1U;
    }

    if(duration_ms > COMM_IMU_STAT_MS_MAX)
    {
        duration_ms = COMM_IMU_STAT_MS_MAX;
    }

    if(comm_read_imu_scaled(&imu, NULL, NULL, NULL, NULL, NULL, NULL, &yaw_x10) != AI_OK)
    {
        comm_send_line("ERR imu no_data");
        return;
    }

    comm_stream_mode = COMM_STREAM_OFF;
    memset(&comm_imu_stat, 0, sizeof(comm_imu_stat));
    comm_imu_stat.active = 1U;
    comm_imu_stat.remaining_ms = duration_ms;
    comm_imu_stat.yaw_start_x10 = yaw_x10;
    comm_imu_stat.yaw_last_x10 = yaw_x10;

    comm_send_line("OK imu stat started ms=%lu stream=off", (unsigned long)duration_ms);
}

static void comm_update_imu_stat(void)
{
    Imu660raScaledData imu;
    int32_t acc_x_mg;
    int32_t acc_y_mg;
    int32_t acc_z_mg;
    int32_t gyro_x_x10;
    int32_t gyro_y_x10;
    int32_t gyro_z_x10;
    int32_t yaw_x10;
    int32_t acc_norm_mg;

    if(comm_imu_stat.active == 0U)
    {
        return;
    }

    if(comm_read_imu_scaled(&imu,
                            &acc_x_mg,
                            &acc_y_mg,
                            &acc_z_mg,
                            &gyro_x_x10,
                            &gyro_y_x10,
                            &gyro_z_x10,
                            &yaw_x10) != AI_OK)
    {
        comm_imu_stat.active = 0U;
        comm_send_line("ERR imu stat no_data");
        return;
    }

    acc_norm_mg = comm_acc_norm_mg(acc_x_mg, acc_y_mg, acc_z_mg);
    comm_imu_stat.acc_norm_mg_sum += acc_norm_mg;
    comm_imu_stat.gyro_x_x10_sum += gyro_x_x10;
    comm_imu_stat.gyro_y_x10_sum += gyro_y_x10;
    comm_imu_stat.gyro_z_x10_sum += gyro_z_x10;
    comm_imu_stat.yaw_last_x10 = yaw_x10;
    comm_imu_stat.samples++;

    if(comm_imu_stat.remaining_ms > AI_COMM_PERIOD_MS)
    {
        comm_imu_stat.remaining_ms -= AI_COMM_PERIOD_MS;
        return;
    }

    if(comm_imu_stat.samples == 0U)
    {
        comm_imu_stat.active = 0U;
        comm_send_line("ERR imu stat no_samples");
        return;
    }

    comm_send_line("OK imu stat samples=%lu acc_norm_mg=%ld gyro_x_x10=%ld gyro_y_x10=%ld gyro_z_x10=%ld yaw_drift_x10=%ld",
                   (unsigned long)comm_imu_stat.samples,
                   (long)(comm_imu_stat.acc_norm_mg_sum / (int64_t)comm_imu_stat.samples),
                   (long)(comm_imu_stat.gyro_x_x10_sum / (int64_t)comm_imu_stat.samples),
                   (long)(comm_imu_stat.gyro_y_x10_sum / (int64_t)comm_imu_stat.samples),
                   (long)(comm_imu_stat.gyro_z_x10_sum / (int64_t)comm_imu_stat.samples),
                   (long)(comm_imu_stat.yaw_last_x10 - comm_imu_stat.yaw_start_x10));
    comm_imu_stat.active = 0U;
}

static void comm_update_enc100(void)
{
    DriveEncoderTotal total;

    comm_enc100_elapsed_ms += AI_COMM_PERIOD_MS;
    if(comm_enc100_elapsed_ms < 100U)
    {
        return;
    }

    comm_enc100_elapsed_ms = 0;

    if(DriveGetAllEncoderTotal(&total) != DRIVE_STATUS_OK)
    {
        return;
    }

    comm_enc100_delta.wheel1 = comm_saturate_i16(total.wheel1 - comm_enc100_last_total.wheel1);
    comm_enc100_delta.wheel2 = comm_saturate_i16(total.wheel2 - comm_enc100_last_total.wheel2);
    comm_enc100_delta.wheel3 = comm_saturate_i16(total.wheel3 - comm_enc100_last_total.wheel3);
    comm_enc100_delta.wheel4 = comm_saturate_i16(total.wheel4 - comm_enc100_last_total.wheel4);
    comm_enc100_last_total = total;
}

static void comm_send_stream_frame(void)
{
    Imu660raScaledData imu;
    DriveEncoderDelta encoder_delta;
    mecanum_duty_t duty;
    int32_t acc_x_mg;
    int32_t acc_y_mg;
    int32_t acc_z_mg;
    int32_t gyro_x_x10;
    int32_t gyro_y_x10;
    int32_t gyro_z_x10;
    int32_t yaw_x10;

    if(comm_stream_mode == COMM_STREAM_OFF)
    {
        return;
    }

    if(comm_stream_mode == COMM_STREAM_IMU_ACC)
    {
        if(comm_read_imu_scaled(&imu, &acc_x_mg, &acc_y_mg, &acc_z_mg, NULL, NULL, NULL, NULL) == AI_OK)
        {
            comm_send_line("DATA imu_acc ax_mg=%ld ay_mg=%ld az_mg=%ld norm_mg=%ld",
                           (long)acc_x_mg,
                           (long)acc_y_mg,
                           (long)acc_z_mg,
                           (long)comm_acc_norm_mg(acc_x_mg, acc_y_mg, acc_z_mg));
        }
    }
    else if(comm_stream_mode == COMM_STREAM_IMU_GYRO)
    {
        if(comm_read_imu_scaled(&imu, NULL, NULL, NULL, &gyro_x_x10, &gyro_y_x10, &gyro_z_x10, &yaw_x10) == AI_OK)
        {
            comm_send_line("DATA imu_gyro gx_x10=%ld gy_x10=%ld gz_x10=%ld yaw_x10=%ld",
                           (long)gyro_x_x10,
                           (long)gyro_y_x10,
                           (long)gyro_z_x10,
                           (long)yaw_x10);
        }
    }
    else if(comm_stream_mode == COMM_STREAM_ENC5)
    {
        if(DriveGetAllEncoderDelta(&encoder_delta) == DRIVE_STATUS_OK)
        {
            comm_send_line("DATA enc5 w1=%d w2=%d w3=%d w4=%d",
                           (int)encoder_delta.wheel1,
                           (int)encoder_delta.wheel2,
                           (int)encoder_delta.wheel3,
                           (int)encoder_delta.wheel4);
        }
    }
    else if(comm_stream_mode == COMM_STREAM_ENC100)
    {
        comm_send_line("DATA enc100 w1=%d w2=%d w3=%d w4=%d",
                       (int)comm_enc100_delta.wheel1,
                       (int)comm_enc100_delta.wheel2,
                       (int)comm_enc100_delta.wheel3,
                       (int)comm_enc100_delta.wheel4);
    }
    else if(comm_stream_mode == COMM_STREAM_DUTY)
    {
        motion_test_get_duty(&duty);
        comm_send_line("DATA duty m1=%d m2=%d m3=%d m4=%d",
                       (int)duty.motor1,
                       (int)duty.motor2,
                       (int)duty.motor3,
                       (int)duty.motor4);
    }
}

static void comm_handle_stream_command(char *mode)
{
    DriveEncoderTotal total;

    if(mode == NULL)
    {
        comm_send_line("ERR stream missing_mode");
        return;
    }

    if(strcmp(mode, "off") == 0)
    {
        comm_stream_mode = COMM_STREAM_OFF;
    }
    else if(strcmp(mode, "imu_acc") == 0)
    {
        comm_stream_mode = COMM_STREAM_IMU_ACC;
    }
    else if(strcmp(mode, "imu_gyro") == 0)
    {
        comm_stream_mode = COMM_STREAM_IMU_GYRO;
    }
    else if(strcmp(mode, "enc5") == 0)
    {
        comm_stream_mode = COMM_STREAM_ENC5;
    }
    else if(strcmp(mode, "enc100") == 0)
    {
        if(DriveGetAllEncoderTotal(&total) == DRIVE_STATUS_OK)
        {
            comm_enc100_last_total = total;
        }

        comm_enc100_delta.wheel1 = 0;
        comm_enc100_delta.wheel2 = 0;
        comm_enc100_delta.wheel3 = 0;
        comm_enc100_delta.wheel4 = 0;
        comm_enc100_elapsed_ms = 0;
        comm_stream_mode = COMM_STREAM_ENC100;
    }
    else if(strcmp(mode, "duty") == 0)
    {
        comm_stream_mode = COMM_STREAM_DUTY;
    }
    else
    {
        comm_send_line("ERR stream bad_mode=%s", mode);
        return;
    }

    comm_send_line("OK stream %s", comm_stream_name(comm_stream_mode));
}

static void comm_handle_imu_command(char *sub_command)
{
    int32_t duration_ms;

    if(sub_command == NULL)
    {
        comm_send_line("ERR imu missing_command");
        return;
    }

    if(strcmp(sub_command, "zero") == 0)
    {
        Imu660raResetYaw();
        comm_send_line("OK imu zero");
    }
    else if(strcmp(sub_command, "stat") == 0)
    {
        char *duration_text = strtok(NULL, " ");

        if(comm_parse_i32(duration_text, &duration_ms) != AI_OK)
        {
            comm_send_line("ERR imu stat bad_ms");
            return;
        }

        if(duration_ms <= 0)
        {
            comm_send_line("ERR imu stat bad_ms");
            return;
        }

        comm_start_imu_stat((uint32_t)duration_ms);
    }
    else
    {
        comm_send_line("ERR imu bad_command=%s", sub_command);
    }
}

static void comm_handle_motor_command(char *wheel_text)
{
    char *duty_text = strtok(NULL, " ");
    char *ms_text = strtok(NULL, " ");
    int32_t duty;
    int32_t run_ms;
    int32_t wheel_id;
    ai_status_t status;

    if((wheel_text == NULL) ||
       (comm_parse_i32(duty_text, &duty) != AI_OK) ||
       (comm_parse_i32(ms_text, &run_ms) != AI_OK) ||
       (comm_validate_duty_arg(duty) != AI_OK) ||
       (comm_validate_run_ms_arg(run_ms) != AI_OK))
    {
        comm_send_line("ERR motor usage");
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
    else if(comm_parse_i32(wheel_text, &wheel_id) == AI_OK)
    {
        status = motion_test_set_motor((uint8_t)wheel_id, (int16_t)duty, (uint32_t)run_ms);
    }
    else
    {
        status = AI_ERR_INVALID_ARG;
    }

    if(status == AI_OK)
    {
        comm_send_line("OK motor %s duty=%ld ms=%ld", wheel_text, (long)duty, (long)run_ms);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_send_line("ERR motor disarmed, send arm");
    }
    else
    {
        comm_send_line("ERR motor invalid_arg");
    }
}

static void comm_handle_move_command(char *move_text)
{
    char *duty_text = strtok(NULL, " ");
    char *ms_text = strtok(NULL, " ");
    int32_t duty;
    int32_t run_ms;
    mecanum_velocity_t velocity;

    if((move_text == NULL) ||
       (comm_parse_i32(duty_text, &duty) != AI_OK) ||
       (comm_parse_i32(ms_text, &run_ms) != AI_OK) ||
       (comm_validate_move_duty_arg(duty) != AI_OK) ||
       (comm_validate_run_ms_arg(run_ms) != AI_OK))
    {
        comm_send_line("ERR move usage");
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
        comm_send_line("ERR move bad_mode=%s", move_text);
        return;
    }

    if(motion_test_set_mecanum(&velocity, (uint32_t)run_ms) == AI_OK)
    {
        comm_send_line("OK move %s duty=%ld ms=%ld", move_text, (long)duty, (long)run_ms);
    }
    else if(motion_test_is_armed() == 0U)
    {
        comm_send_line("ERR move disarmed, send arm");
    }
    else
    {
        comm_send_line("ERR move invalid_arg");
    }
}

static void comm_handle_line(char *line)
{
    char *command = strtok(line, " ");

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
        (void)motion_test_stop();
        comm_send_line("OK stop disarmed");
    }
    else if(strcmp(command, "arm") == 0)
    {
        (void)motion_test_arm();
        comm_send_line("OK arm");
    }
    else if(strcmp(command, "disarm") == 0)
    {
        (void)motion_test_disarm();
        comm_send_line("OK disarm");
    }
    else if(strcmp(command, "status") == 0)
    {
        comm_send_line("OK status armed=%u stream=%s",
                       (unsigned int)motion_test_is_armed(),
                       comm_stream_name(comm_stream_mode));
    }
    else if(strcmp(command, "stream") == 0)
    {
        comm_handle_stream_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "imu") == 0)
    {
        comm_handle_imu_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "motor") == 0)
    {
        comm_handle_motor_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "move") == 0)
    {
        comm_handle_move_command(strtok(NULL, " "));
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
    comm_send_line("OK open_loop_test ready uart=UART1 baud=115200 tick=%lu", (unsigned long)comm_ready_report_count);
}

ai_status_t comm_module_init(void)
{
    DriveEncoderTotal total;

    comm_line_length = 0;
    comm_line_idle_ms = 0;
    comm_line_overflow = 0;
    comm_command_seen = 0;
    comm_ready_report_elapsed_ms = 0;
    comm_ready_report_count = 0;
    comm_stream_mode = COMM_STREAM_OFF;
    comm_enc100_elapsed_ms = 0;
    comm_enc100_delta.wheel1 = 0;
    comm_enc100_delta.wheel2 = 0;
    comm_enc100_delta.wheel3 = 0;
    comm_enc100_delta.wheel4 = 0;
    memset(&comm_imu_stat, 0, sizeof(comm_imu_stat));

    if(DriveGetAllEncoderTotal(&total) == DRIVE_STATUS_OK)
    {
        comm_enc100_last_total = total;
    }
    else
    {
        comm_enc100_last_total.wheel1 = 0;
        comm_enc100_last_total.wheel2 = 0;
        comm_enc100_last_total.wheel3 = 0;
        comm_enc100_last_total.wheel4 = 0;
    }

    comm_send_line("OK open_loop_test ready uart=UART1 baud=115200, type help, send arm before motor/move");

    return WirelessInit();
}

void comm_module_tick(void)
{
    comm_poll_rx();
    comm_update_idle_line();
    comm_update_ready_report();
    comm_update_enc100();
    comm_update_imu_stat();
    comm_send_stream_frame();
}
