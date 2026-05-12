#include "comm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_config.h"
#include "calibration/calibration_telemetry.h"
#include "drive.h"
#include "imu660ra.h"
#include "motion.h"
#include "vision.h"
#include "vision_protocol.h"
#include "wireless.h"
#include "zf_common_debug.h"

#define COMM_LINE_BUFFER_SIZE        (96U)
#define COMM_RX_BUFFER_SIZE          (32U)
#define COMM_RESPONSE_BUFFER_SIZE    (384U)
#define COMM_IMU_STAT_MS_MAX         (60000U)
#define COMM_COMMAND_IDLE_MS         (200U)
#define COMM_READY_REPORT_PERIOD_MS  (1000U)

typedef enum
{
    COMM_STREAM_OFF = 0,
    COMM_STREAM_IMU_ACC,
    COMM_STREAM_IMU_GYRO,
    COMM_STREAM_IMU_YAWX,
    COMM_STREAM_ENC5,
    COMM_STREAM_ENC100,
    COMM_STREAM_DUTY,
    COMM_STREAM_SPEED,
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
static calibration_encoder_state_t comm_cal_encoder;
static calibration_yawx_state_t comm_cal_yawx;
static comm_imu_stat_t comm_imu_stat;
static uint8_t comm_vision_sim_seq;

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

static ai_status_t comm_parse_f32(const char *text, float *value)
{
    char *end_ptr;
    float parsed;

    if((text == NULL) || (value == NULL) || (text[0] == '\0'))
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

static ai_status_t comm_parse_u8_auto(const char *text, uint8_t *value)
{
    char *end_ptr;
    unsigned long parsed;

    if((text == NULL) || (value == NULL) || (text[0] == '\0'))
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

    if(mode == COMM_STREAM_IMU_YAWX)
    {
        return "imu_yawx";
    }

    if(mode == COMM_STREAM_ENC100)
    {
        return "enc100";
    }

    if(mode == COMM_STREAM_DUTY)
    {
        return "duty";
    }

    if(mode == COMM_STREAM_SPEED)
    {
        return "speed";
    }

    return "off";
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
    comm_send_line("motor <1|2|3|4|all> <duty_pct> <ms>");
    comm_send_line("move <fwd|back|left|right|ccw|cw> <duty_pct> <ms>");
    comm_send_line("vision");
    comm_send_line("vision clear");
    comm_send_line("vision sim move <up|down|left|right> <cm> [seq]");
    comm_send_line("vision sim rotate <ccw|cw> <90|180> [seq]");
    comm_send_line("vision sim stop|query|reset [seq]");
    comm_send_line("vision sim raw <seq> <cmd> <dir> <val>");
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

static void comm_encoder_total_to_counts(const DriveEncoderTotal *total,
                                         calibration_wheel_counts_t *counts)
{
    counts->wheel1 = total->wheel1;
    counts->wheel2 = total->wheel2;
    counts->wheel3 = total->wheel3;
    counts->wheel4 = total->wheel4;
}

static ai_status_t comm_read_encoder_counts(calibration_wheel_counts_t *counts)
{
    DriveEncoderTotal total;

    if(counts == NULL)
    {
        return AI_ERR_INVALID_ARG;
    }

    if(DriveGetAllEncoderTotal(&total) != DRIVE_STATUS_OK)
    {
        return AI_ERR_NO_DATA;
    }

    comm_encoder_total_to_counts(&total, counts);
    return AI_OK;
}

static void comm_send_encoder_counts(const char *label, const calibration_wheel_counts_t *counts)
{
    comm_send_line("DATA %s w1=%ld w2=%ld w3=%ld w4=%ld",
                   label,
                   (long)counts->wheel1,
                   (long)counts->wheel2,
                   (long)counts->wheel3,
                   (long)counts->wheel4);
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
    motion_speed_sample_t speed_sample;
    calibration_yawx_sample_t yawx_sample;
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
    else if(comm_stream_mode == COMM_STREAM_IMU_YAWX)
    {
        if(comm_read_imu_scaled(&imu, NULL, NULL, NULL, &gyro_x_x10, NULL, NULL, &yaw_x10) == AI_OK)
        {
            if(calibration_yawx_sample(&comm_cal_yawx, gyro_x_x10, yaw_x10, &yawx_sample) == AI_OK)
            {
                comm_send_line("DATA imu_yawx gx_x10=%ld yawx_x10=%ld",
                               (long)yawx_sample.gx_x10,
                               (long)yawx_sample.yawx_x10);
            }
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
    else if(comm_stream_mode == COMM_STREAM_SPEED)
    {
        motion_speed_bench_get_sample(&speed_sample);
        comm_send_line("DATA speed dt_ms=%lu t1=%.1f m1=%.1f d1=%.1f e1=%ld t2=%.1f m2=%.1f d2=%.1f e2=%ld t3=%.1f m3=%.1f d3=%.1f e3=%ld t4=%.1f m4=%.1f d4=%.1f e4=%ld",
                       (unsigned long)speed_sample.dt_ms,
                       (double)speed_sample.target_mm_s.wheel1,
                       (double)speed_sample.measured_mm_s.wheel1,
                       (double)speed_sample.duty_percent.wheel1,
                       (long)speed_sample.encoder_delta.wheel1,
                       (double)speed_sample.target_mm_s.wheel2,
                       (double)speed_sample.measured_mm_s.wheel2,
                       (double)speed_sample.duty_percent.wheel2,
                       (long)speed_sample.encoder_delta.wheel2,
                       (double)speed_sample.target_mm_s.wheel3,
                       (double)speed_sample.measured_mm_s.wheel3,
                       (double)speed_sample.duty_percent.wheel3,
                       (long)speed_sample.encoder_delta.wheel3,
                       (double)speed_sample.target_mm_s.wheel4,
                       (double)speed_sample.measured_mm_s.wheel4,
                       (double)speed_sample.duty_percent.wheel4,
                       (long)speed_sample.encoder_delta.wheel4);
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
    else if(strcmp(mode, "imu_yawx") == 0)
    {
        comm_stream_mode = COMM_STREAM_IMU_YAWX;
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
    else if(strcmp(mode, "speed") == 0)
    {
        comm_stream_mode = COMM_STREAM_SPEED;
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
        calibration_yawx_zero(&comm_cal_yawx, 0);
        comm_send_line("OK imu zero");
    }
    else if(strcmp(sub_command, "yawx") == 0)
    {
        char *yawx_command = strtok(NULL, " ");

        if((yawx_command != NULL) && (strcmp(yawx_command, "zero") == 0))
        {
            Imu660raResetYaw();
            calibration_yawx_zero(&comm_cal_yawx, 0);
            comm_send_line("OK imu yawx zero");
        }
        else
        {
            comm_send_line("ERR imu yawx usage");
        }
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

static void comm_handle_cal_enc_command(char *enc_command)
{
    calibration_wheel_counts_t counts;
    calibration_wheel_counts_t delta;
    calibration_encoder_turn_result_t result;
    char *wheel_text;
    char *turns_text;
    int32_t wheel_id;
    int32_t turns;

    if(enc_command == NULL)
    {
        comm_send_line("ERR cal enc usage");
        return;
    }

    if(strcmp(enc_command, "zero") == 0)
    {
        if(comm_read_encoder_counts(&counts) != AI_OK)
        {
            comm_send_line("ERR cal enc no_data");
            return;
        }

        calibration_encoder_reset_baseline(&comm_cal_encoder, &counts);
        comm_send_line("OK cal enc zero w1=%ld w2=%ld w3=%ld w4=%ld",
                       (long)counts.wheel1,
                       (long)counts.wheel2,
                       (long)counts.wheel3,
                       (long)counts.wheel4);
    }
    else if(strcmp(enc_command, "total") == 0)
    {
        if(comm_read_encoder_counts(&counts) != AI_OK)
        {
            comm_send_line("ERR cal enc no_data");
            return;
        }

        comm_send_encoder_counts("cal_enc_total", &counts);
    }
    else if(strcmp(enc_command, "delta") == 0)
    {
        if(comm_read_encoder_counts(&counts) != AI_OK)
        {
            comm_send_line("ERR cal enc no_data");
            return;
        }

        if(calibration_encoder_delta(&comm_cal_encoder, &counts, &delta) != AI_OK)
        {
            comm_send_line("ERR cal enc no_data");
            return;
        }

        comm_send_encoder_counts("cal_enc_delta", &delta);
    }
    else if(strcmp(enc_command, "wheel") == 0)
    {
        wheel_text = strtok(NULL, " ");
        turns_text = strtok(NULL, " ");

        if(comm_parse_i32(wheel_text, &wheel_id) != AI_OK)
        {
            comm_send_line("ERR cal enc bad_wheel");
            return;
        }

        if((wheel_id < 1) || (wheel_id > 4))
        {
            comm_send_line("ERR cal enc bad_wheel");
            return;
        }

        if((comm_parse_i32(turns_text, &turns) != AI_OK) || (turns <= 0))
        {
            comm_send_line("ERR cal enc bad_turns");
            return;
        }

        if(comm_read_encoder_counts(&counts) != AI_OK)
        {
            comm_send_line("ERR cal enc no_data");
            return;
        }

        if(calibration_encoder_counts_per_rev(&comm_cal_encoder,
                                              &counts,
                                              (uint8_t)wheel_id,
                                              (uint32_t)turns,
                                              &result) != AI_OK)
        {
            comm_send_line("ERR cal enc bad_arg");
            return;
        }

        comm_send_line("OK cal enc wheel=%ld counts=%ld turns=%lu counts_per_rev_x100=%ld",
                       (long)wheel_id,
                       (long)result.counts,
                       (unsigned long)result.turns,
                       (long)result.counts_per_rev_x100);
    }
    else
    {
        comm_send_line("ERR cal enc usage");
    }
}

static void comm_handle_cal_command(char *sub_command)
{
    if(sub_command == NULL)
    {
        comm_send_line("ERR cal usage");
        return;
    }

    if(strcmp(sub_command, "enc") == 0)
    {
        comm_handle_cal_enc_command(strtok(NULL, " "));
    }
    else
    {
        comm_send_line("ERR cal usage");
    }
}

static void comm_handle_speed_command(char *sub_command)
{
    ai_status_t status;

    if(sub_command == NULL)
    {
        comm_send_line("ERR speed usage");
        return;
    }

    if(strcmp(sub_command, "arm") == 0)
    {
        status = motion_speed_bench_arm();
        if(status == AI_OK)
        {
            comm_send_line("OK speed arm");
        }
        else
        {
            comm_send_line("ERR speed arm");
        }
    }
    else if(strcmp(sub_command, "stop") == 0)
    {
        (void)motion_speed_bench_stop();
        comm_send_line("OK speed stop");
    }
    else if(strcmp(sub_command, "wheel") == 0)
    {
        char *wheel_text = strtok(NULL, " ");
        char *speed_text = strtok(NULL, " ");
        int32_t wheel_id;
        float target_mm_s;

        if((comm_parse_i32(wheel_text, &wheel_id) != AI_OK) ||
           (comm_parse_f32(speed_text, &target_mm_s) != AI_OK))
        {
            comm_send_line("ERR speed wheel usage");
            return;
        }

        status = motion_speed_bench_set_wheel((uint8_t)wheel_id, target_mm_s);
        if(status == AI_OK)
        {
            comm_send_line("OK speed wheel=%ld target_mm_s=%.1f", (long)wheel_id, (double)target_mm_s);
        }
        else if(status == AI_ERR_BUSY)
        {
            comm_send_line("ERR speed disarmed, send speed arm");
        }
        else
        {
            comm_send_line("ERR speed wheel bad_arg");
        }
    }
    else if(strcmp(sub_command, "all") == 0)
    {
        char *w1_text = strtok(NULL, " ");
        char *w2_text = strtok(NULL, " ");
        char *w3_text = strtok(NULL, " ");
        char *w4_text = strtok(NULL, " ");
        float w1;
        float w2;
        float w3;
        float w4;

        if((comm_parse_f32(w1_text, &w1) != AI_OK) ||
           (comm_parse_f32(w2_text, &w2) != AI_OK) ||
           (comm_parse_f32(w3_text, &w3) != AI_OK) ||
           (comm_parse_f32(w4_text, &w4) != AI_OK))
        {
            comm_send_line("ERR speed all usage");
            return;
        }

        status = motion_speed_bench_set_all(w1, w2, w3, w4);
        if(status == AI_OK)
        {
            comm_send_line("OK speed all w1=%.1f w2=%.1f w3=%.1f w4=%.1f",
                           (double)w1,
                           (double)w2,
                           (double)w3,
                           (double)w4);
        }
        else if(status == AI_ERR_BUSY)
        {
            comm_send_line("ERR speed disarmed, send speed arm");
        }
        else
        {
            comm_send_line("ERR speed all bad_arg");
        }
    }
    else if(strcmp(sub_command, "pid") == 0)
    {
        char *kp_text = strtok(NULL, " ");
        char *ki_text = strtok(NULL, " ");
        char *kd_text = strtok(NULL, " ");
        float kp;
        float ki;
        float kd;

        if((comm_parse_f32(kp_text, &kp) != AI_OK) ||
           (comm_parse_f32(ki_text, &ki) != AI_OK) ||
           (comm_parse_f32(kd_text, &kd) != AI_OK))
        {
            comm_send_line("ERR speed pid usage");
            return;
        }

        if(motion_speed_bench_set_pid(kp, ki, kd) == AI_OK)
        {
            comm_send_line("OK speed pid kp=%.4f ki=%.4f kd=%.4f", (double)kp, (double)ki, (double)kd);
        }
        else
        {
            comm_send_line("ERR speed pid bad_arg");
        }
    }
    else if(strcmp(sub_command, "limit") == 0)
    {
        char *duty_text = strtok(NULL, " ");
        char *speed_text = strtok(NULL, " ");
        float duty_limit;
        float max_speed;

        if((comm_parse_f32(duty_text, &duty_limit) != AI_OK) ||
           (comm_parse_f32(speed_text, &max_speed) != AI_OK))
        {
            comm_send_line("ERR speed limit usage");
            return;
        }

        if(motion_speed_bench_set_limits(duty_limit, max_speed) == AI_OK)
        {
            comm_send_line("OK speed limit duty=%.1f max_mm_s=%.1f",
                           (double)duty_limit,
                           (double)max_speed);
        }
        else
        {
            comm_send_line("ERR speed limit bad_arg");
        }
    }
    else
    {
        comm_send_line("ERR speed usage");
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

static void comm_send_vision_sim_result(ai_status_t status, const uint8_t *frame, uint8_t explicit_seq)
{
    if(status == AI_OK)
    {
        if(explicit_seq == 0U)
        {
            comm_vision_sim_seq++;
        }

        comm_send_line("OK vision sim %02X %02X %02X %02X %02X %02X %02X",
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
        comm_send_line("ERR vision sim busy");
    }
    else
    {
        comm_send_line("ERR vision sim bad_arg");
    }
}

static void comm_inject_vision_sim_frame(uint8_t seq,
                                         uint8_t cmd,
                                         uint8_t dir,
                                         uint8_t val,
                                         uint8_t explicit_seq)
{
    uint8_t frame[VISION_PROTOCOL_FRAME_LENGTH];
    ai_status_t status;

    status = vision_protocol_inject_frame(seq, cmd, dir, val, frame);
    comm_send_vision_sim_result(status, frame, explicit_seq);
}

static ai_status_t comm_get_vision_sim_seq(char *seq_text, uint8_t *seq, uint8_t *explicit_seq)
{
    if((seq == NULL) || (explicit_seq == NULL))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(seq_text == NULL)
    {
        *seq = comm_vision_sim_seq;
        *explicit_seq = 0U;
        return AI_OK;
    }

    if(comm_parse_u8_auto(seq_text, seq) != AI_OK)
    {
        return AI_ERR_INVALID_ARG;
    }

    *explicit_seq = 1U;
    return AI_OK;
}

static ai_status_t comm_parse_vision_sim_move_dir(const char *dir_text, uint8_t *dir)
{
    if((dir_text == NULL) || (dir == NULL))
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

static ai_status_t comm_parse_vision_sim_rotate_dir(const char *dir_text, uint8_t *dir)
{
    if((dir_text == NULL) || (dir == NULL))
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

static ai_status_t comm_parse_vision_sim_angle(const char *angle_text, uint8_t *val)
{
    int32_t angle;

    if((val == NULL) || (comm_parse_i32(angle_text, &angle) != AI_OK))
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

static void comm_handle_vision_sim_move(void)
{
    char *dir_text = strtok(NULL, " ");
    char *cm_text = strtok(NULL, " ");
    char *seq_text = strtok(NULL, " ");
    uint8_t seq;
    uint8_t explicit_seq;
    uint8_t dir;
    int32_t cm;

    if((dir_text == NULL) ||
       (cm_text == NULL) ||
       (comm_parse_vision_sim_move_dir(dir_text, &dir) != AI_OK) ||
       (comm_parse_i32(cm_text, &cm) != AI_OK) ||
       (cm <= 0) ||
       (cm > UINT8_MAX) ||
       (comm_get_vision_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK))
    {
        comm_send_line("ERR vision sim bad_arg");
        return;
    }

    comm_inject_vision_sim_frame(seq, VISION_PROTOCOL_CMD_MOVE, dir, (uint8_t)cm, explicit_seq);
}

static void comm_handle_vision_sim_rotate(void)
{
    char *dir_text = strtok(NULL, " ");
    char *angle_text = strtok(NULL, " ");
    char *seq_text = strtok(NULL, " ");
    uint8_t seq;
    uint8_t explicit_seq;
    uint8_t dir;
    uint8_t val;

    if((dir_text == NULL) ||
       (angle_text == NULL) ||
       (comm_parse_vision_sim_rotate_dir(dir_text, &dir) != AI_OK) ||
       (comm_parse_vision_sim_angle(angle_text, &val) != AI_OK) ||
       (comm_get_vision_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK))
    {
        comm_send_line("ERR vision sim bad_arg");
        return;
    }

    comm_inject_vision_sim_frame(seq, VISION_PROTOCOL_CMD_ROTATE, dir, val, explicit_seq);
}

static void comm_handle_vision_sim_simple(uint8_t cmd)
{
    char *seq_text = strtok(NULL, " ");
    uint8_t seq;
    uint8_t explicit_seq;

    if(comm_get_vision_sim_seq(seq_text, &seq, &explicit_seq) != AI_OK)
    {
        comm_send_line("ERR vision sim bad_arg");
        return;
    }

    comm_inject_vision_sim_frame(seq, cmd, 0U, 0U, explicit_seq);
}

static void comm_handle_vision_sim_raw(void)
{
    char *seq_text = strtok(NULL, " ");
    char *cmd_text = strtok(NULL, " ");
    char *dir_text = strtok(NULL, " ");
    char *val_text = strtok(NULL, " ");
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;

    if((comm_parse_u8_auto(seq_text, &seq) != AI_OK) ||
       (comm_parse_u8_auto(cmd_text, &cmd) != AI_OK) ||
       (comm_parse_u8_auto(dir_text, &dir) != AI_OK) ||
       (comm_parse_u8_auto(val_text, &val) != AI_OK))
    {
        comm_send_line("ERR vision sim bad_arg");
        return;
    }

    comm_inject_vision_sim_frame(seq, cmd, dir, val, 1U);
}

static void comm_handle_vision_sim_command(void)
{
    char *sim_command = strtok(NULL, " ");

    if(sim_command == NULL)
    {
        comm_send_line("ERR vision sim usage");
        return;
    }

    if(strcmp(sim_command, "move") == 0)
    {
        comm_handle_vision_sim_move();
    }
    else if(strcmp(sim_command, "rotate") == 0)
    {
        comm_handle_vision_sim_rotate();
    }
    else if(strcmp(sim_command, "stop") == 0)
    {
        comm_handle_vision_sim_simple(VISION_PROTOCOL_CMD_STOP);
    }
    else if(strcmp(sim_command, "query") == 0)
    {
        comm_handle_vision_sim_simple(VISION_PROTOCOL_CMD_QUERY);
    }
    else if(strcmp(sim_command, "reset") == 0)
    {
        comm_handle_vision_sim_simple(VISION_PROTOCOL_CMD_RESET);
    }
    else if(strcmp(sim_command, "raw") == 0)
    {
        comm_handle_vision_sim_raw();
    }
    else
    {
        comm_send_line("ERR vision sim usage");
    }
}

static void comm_handle_vision_command(char *sub_command)
{
    vision_debug_t debug;

    if((sub_command != NULL) && (strcmp(sub_command, "sim") == 0))
    {
        comm_handle_vision_sim_command();
        return;
    }

    if((sub_command != NULL) && (strcmp(sub_command, "clear") == 0))
    {
        vision_debug_clear();
        comm_send_line("OK vision clear");
        return;
    }

    vision_debug_get(&debug);
    comm_send_line("OK vision status=%u parser=%u active_seq=%u rx_bytes=%lu frames_ok=%lu bad_frame=%lu bad_cmd=%lu busy=%lu motion_err=%lu",
                   (unsigned int)debug.status,
                   (unsigned int)debug.parser_state,
                   (unsigned int)debug.active_seq,
                   (unsigned long)debug.rx_bytes,
                   (unsigned long)debug.frames_ok,
                   (unsigned long)debug.bad_frames,
                   (unsigned long)debug.bad_cmds,
                   (unsigned long)debug.busy_errors,
                   (unsigned long)debug.motion_errors);
    comm_send_line("OK vision cmd move=%lu rotate=%lu stop=%lu query=%lu reset=%lu last=%02X %02X %02X %02X err=%02X:%02X:%02X ovf=%lu",
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
    comm_send_line("DATA vision_rx count=%u %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
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
        motion_action_stop_all();
        comm_send_line("OK stop");
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
        comm_send_line("OK status mode=%s armed=%u speed_armed=%u stream=%s",
                       comm_motion_mode_name(motion_get_mode()),
                       (unsigned int)motion_test_is_armed(),
                       (unsigned int)motion_speed_bench_is_armed(),
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
    else if(strcmp(command, "cal") == 0)
    {
        comm_handle_cal_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "speed") == 0)
    {
        comm_handle_speed_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "motor") == 0)
    {
        comm_handle_motor_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "move") == 0)
    {
        comm_handle_move_command(strtok(NULL, " "));
    }
    else if(strcmp(command, "vision") == 0)
    {
        comm_handle_vision_command(strtok(NULL, " "));
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
    DriveEncoderTotal total;
    calibration_wheel_counts_t counts;

    comm_line_length = 0;
    comm_line_idle_ms = 0;
    comm_line_overflow = 0;
    comm_command_seen = 0;
    comm_ready_report_elapsed_ms = 0;
    comm_ready_report_count = 0;
    comm_vision_sim_seq = 0;
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
        total.wheel1 = 0;
        total.wheel2 = 0;
        total.wheel3 = 0;
        total.wheel4 = 0;
        comm_enc100_last_total = total;
    }

    comm_encoder_total_to_counts(&total, &counts);
    calibration_encoder_reset_baseline(&comm_cal_encoder, &counts);
    calibration_yawx_zero(&comm_cal_yawx, 0);

    comm_send_line("OK closed_loop ready uart=UART1 baud=115200, type help");

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
