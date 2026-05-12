#include "comm_telemetry_debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai_config.h"
#include "ai_error.h"
#include "calibration/calibration_telemetry.h"
#include "drive.h"
#include "imu660ra.h"
#include "motion.h"

#define COMM_TELEMETRY_DEBUG_RESPONSE_BUFFER_SIZE (384U)
#define COMM_TELEMETRY_DEBUG_IMU_STAT_MS_MAX      (60000U)

typedef enum
{
    COMM_TELEMETRY_STREAM_OFF = 0,
    COMM_TELEMETRY_STREAM_IMU_ACC,
    COMM_TELEMETRY_STREAM_IMU_GYRO,
    COMM_TELEMETRY_STREAM_IMU_YAWX,
    COMM_TELEMETRY_STREAM_ENC5,
    COMM_TELEMETRY_STREAM_ENC100,
    COMM_TELEMETRY_STREAM_DUTY,
    COMM_TELEMETRY_STREAM_SPEED,
} comm_telemetry_stream_mode_t;

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
} comm_telemetry_imu_stat_t;

static comm_telemetry_stream_mode_t comm_telemetry_stream_mode;
static DriveEncoderTotal comm_telemetry_enc100_last_total;
static uint32_t comm_telemetry_enc100_elapsed_ms;
static DriveEncoderDelta comm_telemetry_enc100_delta;
static calibration_yawx_state_t comm_telemetry_cal_yawx;
static comm_telemetry_imu_stat_t comm_telemetry_imu_stat;

static void comm_telemetry_debug_send_line(comm_telemetry_debug_write_line_t write_line,
                                           const char *format,
                                           ...)
{
    char buffer[COMM_TELEMETRY_DEBUG_RESPONSE_BUFFER_SIZE];
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

static int32_t comm_telemetry_debug_float_to_i32(float value)
{
    if(value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}

static int16_t comm_telemetry_debug_saturate_i16(int32_t value)
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

static uint32_t comm_telemetry_debug_isqrt_u64(uint64_t value)
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

static int32_t comm_telemetry_debug_acc_norm_mg(int32_t acc_x_mg,
                                                int32_t acc_y_mg,
                                                int32_t acc_z_mg)
{
    uint64_t sum_square = 0;

    sum_square += (uint64_t)((int64_t)acc_x_mg * acc_x_mg);
    sum_square += (uint64_t)((int64_t)acc_y_mg * acc_y_mg);
    sum_square += (uint64_t)((int64_t)acc_z_mg * acc_z_mg);

    return (int32_t)comm_telemetry_debug_isqrt_u64(sum_square);
}

static ai_status_t comm_telemetry_debug_parse_i32(const char *text, int32_t *value)
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

static const char *comm_telemetry_debug_stream_mode_name(comm_telemetry_stream_mode_t mode)
{
    if(mode == COMM_TELEMETRY_STREAM_IMU_ACC)
    {
        return "imu_acc";
    }

    if(mode == COMM_TELEMETRY_STREAM_IMU_GYRO)
    {
        return "imu_gyro";
    }

    if(mode == COMM_TELEMETRY_STREAM_ENC5)
    {
        return "enc5";
    }

    if(mode == COMM_TELEMETRY_STREAM_IMU_YAWX)
    {
        return "imu_yawx";
    }

    if(mode == COMM_TELEMETRY_STREAM_ENC100)
    {
        return "enc100";
    }

    if(mode == COMM_TELEMETRY_STREAM_DUTY)
    {
        return "duty";
    }

    if(mode == COMM_TELEMETRY_STREAM_SPEED)
    {
        return "speed";
    }

    return "off";
}

static ai_status_t comm_telemetry_debug_read_imu_scaled(Imu660raScaledData *imu,
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

    if(acc_x_mg != 0)
    {
        *acc_x_mg = comm_telemetry_debug_float_to_i32(imu->accXG * 1000.0f);
    }

    if(acc_y_mg != 0)
    {
        *acc_y_mg = comm_telemetry_debug_float_to_i32(imu->accYG * 1000.0f);
    }

    if(acc_z_mg != 0)
    {
        *acc_z_mg = comm_telemetry_debug_float_to_i32(imu->accZG * 1000.0f);
    }

    if(gyro_x_x10 != 0)
    {
        *gyro_x_x10 = comm_telemetry_debug_float_to_i32(imu->gyroXDps * 10.0f);
    }

    if(gyro_y_x10 != 0)
    {
        *gyro_y_x10 = comm_telemetry_debug_float_to_i32(imu->gyroYDps * 10.0f);
    }

    if(gyro_z_x10 != 0)
    {
        *gyro_z_x10 = comm_telemetry_debug_float_to_i32(imu->gyroZDps * 10.0f);
    }

    if(yaw_x10 != 0)
    {
        *yaw_x10 = comm_telemetry_debug_float_to_i32(imu->yawDeg * 10.0f);
    }

    return AI_OK;
}

static void comm_telemetry_debug_start_imu_stat(uint32_t duration_ms,
                                                comm_telemetry_debug_write_line_t write_line)
{
    Imu660raScaledData imu;
    int32_t yaw_x10;

    if(duration_ms == 0U)
    {
        duration_ms = 1U;
    }

    if(duration_ms > COMM_TELEMETRY_DEBUG_IMU_STAT_MS_MAX)
    {
        duration_ms = COMM_TELEMETRY_DEBUG_IMU_STAT_MS_MAX;
    }

    if(comm_telemetry_debug_read_imu_scaled(&imu, 0, 0, 0, 0, 0, 0, &yaw_x10) != AI_OK)
    {
        comm_telemetry_debug_send_line(write_line, "ERR imu no_data");
        return;
    }

    comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_OFF;
    memset(&comm_telemetry_imu_stat, 0, sizeof(comm_telemetry_imu_stat));
    comm_telemetry_imu_stat.active = 1U;
    comm_telemetry_imu_stat.remaining_ms = duration_ms;
    comm_telemetry_imu_stat.yaw_start_x10 = yaw_x10;
    comm_telemetry_imu_stat.yaw_last_x10 = yaw_x10;

    comm_telemetry_debug_send_line(write_line,
                                   "OK imu stat started ms=%lu stream=off",
                                   (unsigned long)duration_ms);
}

static void comm_telemetry_debug_update_imu_stat(comm_telemetry_debug_write_line_t write_line)
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

    if(comm_telemetry_imu_stat.active == 0U)
    {
        return;
    }

    if(comm_telemetry_debug_read_imu_scaled(&imu,
                                            &acc_x_mg,
                                            &acc_y_mg,
                                            &acc_z_mg,
                                            &gyro_x_x10,
                                            &gyro_y_x10,
                                            &gyro_z_x10,
                                            &yaw_x10) != AI_OK)
    {
        comm_telemetry_imu_stat.active = 0U;
        comm_telemetry_debug_send_line(write_line, "ERR imu stat no_data");
        return;
    }

    acc_norm_mg = comm_telemetry_debug_acc_norm_mg(acc_x_mg, acc_y_mg, acc_z_mg);
    comm_telemetry_imu_stat.acc_norm_mg_sum += acc_norm_mg;
    comm_telemetry_imu_stat.gyro_x_x10_sum += gyro_x_x10;
    comm_telemetry_imu_stat.gyro_y_x10_sum += gyro_y_x10;
    comm_telemetry_imu_stat.gyro_z_x10_sum += gyro_z_x10;
    comm_telemetry_imu_stat.yaw_last_x10 = yaw_x10;
    comm_telemetry_imu_stat.samples++;

    if(comm_telemetry_imu_stat.remaining_ms > AI_COMM_PERIOD_MS)
    {
        comm_telemetry_imu_stat.remaining_ms -= AI_COMM_PERIOD_MS;
        return;
    }

    if(comm_telemetry_imu_stat.samples == 0U)
    {
        comm_telemetry_imu_stat.active = 0U;
        comm_telemetry_debug_send_line(write_line, "ERR imu stat no_samples");
        return;
    }

    comm_telemetry_debug_send_line(write_line,
                                   "OK imu stat samples=%lu acc_norm_mg=%ld gyro_x_x10=%ld gyro_y_x10=%ld gyro_z_x10=%ld yaw_drift_x10=%ld",
                                   (unsigned long)comm_telemetry_imu_stat.samples,
                                   (long)(comm_telemetry_imu_stat.acc_norm_mg_sum / (int64_t)comm_telemetry_imu_stat.samples),
                                   (long)(comm_telemetry_imu_stat.gyro_x_x10_sum / (int64_t)comm_telemetry_imu_stat.samples),
                                   (long)(comm_telemetry_imu_stat.gyro_y_x10_sum / (int64_t)comm_telemetry_imu_stat.samples),
                                   (long)(comm_telemetry_imu_stat.gyro_z_x10_sum / (int64_t)comm_telemetry_imu_stat.samples),
                                   (long)(comm_telemetry_imu_stat.yaw_last_x10 - comm_telemetry_imu_stat.yaw_start_x10));
    comm_telemetry_imu_stat.active = 0U;
}

static void comm_telemetry_debug_update_enc100(void)
{
    DriveEncoderTotal total;

    comm_telemetry_enc100_elapsed_ms += AI_COMM_PERIOD_MS;
    if(comm_telemetry_enc100_elapsed_ms < 100U)
    {
        return;
    }

    comm_telemetry_enc100_elapsed_ms = 0;

    if(DriveGetAllEncoderTotal(&total) != DRIVE_STATUS_OK)
    {
        return;
    }

    comm_telemetry_enc100_delta.wheel1 =
        comm_telemetry_debug_saturate_i16(total.wheel1 - comm_telemetry_enc100_last_total.wheel1);
    comm_telemetry_enc100_delta.wheel2 =
        comm_telemetry_debug_saturate_i16(total.wheel2 - comm_telemetry_enc100_last_total.wheel2);
    comm_telemetry_enc100_delta.wheel3 =
        comm_telemetry_debug_saturate_i16(total.wheel3 - comm_telemetry_enc100_last_total.wheel3);
    comm_telemetry_enc100_delta.wheel4 =
        comm_telemetry_debug_saturate_i16(total.wheel4 - comm_telemetry_enc100_last_total.wheel4);
    comm_telemetry_enc100_last_total = total;
}

static void comm_telemetry_debug_send_stream_frame(comm_telemetry_debug_write_line_t write_line)
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

    if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_OFF)
    {
        return;
    }

    if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_IMU_ACC)
    {
        if(comm_telemetry_debug_read_imu_scaled(&imu, &acc_x_mg, &acc_y_mg, &acc_z_mg, 0, 0, 0, 0) == AI_OK)
        {
            comm_telemetry_debug_send_line(write_line,
                                           "DATA imu_acc ax_mg=%ld ay_mg=%ld az_mg=%ld norm_mg=%ld",
                                           (long)acc_x_mg,
                                           (long)acc_y_mg,
                                           (long)acc_z_mg,
                                           (long)comm_telemetry_debug_acc_norm_mg(acc_x_mg, acc_y_mg, acc_z_mg));
        }
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_IMU_GYRO)
    {
        if(comm_telemetry_debug_read_imu_scaled(&imu, 0, 0, 0, &gyro_x_x10, &gyro_y_x10, &gyro_z_x10, &yaw_x10) == AI_OK)
        {
            comm_telemetry_debug_send_line(write_line,
                                           "DATA imu_gyro gx_x10=%ld gy_x10=%ld gz_x10=%ld yaw_x10=%ld",
                                           (long)gyro_x_x10,
                                           (long)gyro_y_x10,
                                           (long)gyro_z_x10,
                                           (long)yaw_x10);
        }
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_IMU_YAWX)
    {
        if(comm_telemetry_debug_read_imu_scaled(&imu, 0, 0, 0, &gyro_x_x10, 0, 0, &yaw_x10) == AI_OK)
        {
            if(calibration_yawx_sample(&comm_telemetry_cal_yawx, gyro_x_x10, yaw_x10, &yawx_sample) == AI_OK)
            {
                comm_telemetry_debug_send_line(write_line,
                                               "DATA imu_yawx gx_x10=%ld yawx_x10=%ld",
                                               (long)yawx_sample.gx_x10,
                                               (long)yawx_sample.yawx_x10);
            }
        }
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_ENC5)
    {
        if(DriveGetAllEncoderDelta(&encoder_delta) == DRIVE_STATUS_OK)
        {
            comm_telemetry_debug_send_line(write_line,
                                           "DATA enc5 w1=%d w2=%d w3=%d w4=%d",
                                           (int)encoder_delta.wheel1,
                                           (int)encoder_delta.wheel2,
                                           (int)encoder_delta.wheel3,
                                           (int)encoder_delta.wheel4);
        }
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_ENC100)
    {
        comm_telemetry_debug_send_line(write_line,
                                       "DATA enc100 w1=%d w2=%d w3=%d w4=%d",
                                       (int)comm_telemetry_enc100_delta.wheel1,
                                       (int)comm_telemetry_enc100_delta.wheel2,
                                       (int)comm_telemetry_enc100_delta.wheel3,
                                       (int)comm_telemetry_enc100_delta.wheel4);
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_DUTY)
    {
        motion_test_get_duty(&duty);
        comm_telemetry_debug_send_line(write_line,
                                       "DATA duty m1=%d m2=%d m3=%d m4=%d",
                                       (int)duty.motor1,
                                       (int)duty.motor2,
                                       (int)duty.motor3,
                                       (int)duty.motor4);
    }
    else if(comm_telemetry_stream_mode == COMM_TELEMETRY_STREAM_SPEED)
    {
        motion_speed_bench_get_sample(&speed_sample);
        comm_telemetry_debug_send_line(write_line,
                                       "DATA speed dt_ms=%lu t1=%.1f r1=%.1f m1=%.1f d1=%.1f e1=%ld t2=%.1f r2=%.1f m2=%.1f d2=%.1f e2=%ld t3=%.1f r3=%.1f m3=%.1f d3=%.1f e3=%ld t4=%.1f r4=%.1f m4=%.1f d4=%.1f e4=%ld",
                                       (unsigned long)speed_sample.dt_ms,
                                       (double)speed_sample.target_mm_s.wheel1,
                                       (double)speed_sample.raw_measured_mm_s.wheel1,
                                       (double)speed_sample.measured_mm_s.wheel1,
                                       (double)speed_sample.duty_percent.wheel1,
                                       (long)speed_sample.encoder_delta.wheel1,
                                       (double)speed_sample.target_mm_s.wheel2,
                                       (double)speed_sample.raw_measured_mm_s.wheel2,
                                       (double)speed_sample.measured_mm_s.wheel2,
                                       (double)speed_sample.duty_percent.wheel2,
                                       (long)speed_sample.encoder_delta.wheel2,
                                       (double)speed_sample.target_mm_s.wheel3,
                                       (double)speed_sample.raw_measured_mm_s.wheel3,
                                       (double)speed_sample.measured_mm_s.wheel3,
                                       (double)speed_sample.duty_percent.wheel3,
                                       (long)speed_sample.encoder_delta.wheel3,
                                       (double)speed_sample.target_mm_s.wheel4,
                                       (double)speed_sample.raw_measured_mm_s.wheel4,
                                       (double)speed_sample.measured_mm_s.wheel4,
                                       (double)speed_sample.duty_percent.wheel4,
                                       (long)speed_sample.encoder_delta.wheel4);
    }
}

void comm_telemetry_debug_init(void)
{
    comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_OFF;
    comm_telemetry_enc100_elapsed_ms = 0;
    comm_telemetry_enc100_last_total.wheel1 = 0;
    comm_telemetry_enc100_last_total.wheel2 = 0;
    comm_telemetry_enc100_last_total.wheel3 = 0;
    comm_telemetry_enc100_last_total.wheel4 = 0;
    comm_telemetry_enc100_delta.wheel1 = 0;
    comm_telemetry_enc100_delta.wheel2 = 0;
    comm_telemetry_enc100_delta.wheel3 = 0;
    comm_telemetry_enc100_delta.wheel4 = 0;
    calibration_yawx_zero(&comm_telemetry_cal_yawx, 0);
    memset(&comm_telemetry_imu_stat, 0, sizeof(comm_telemetry_imu_stat));
}

const char *comm_telemetry_debug_stream_name(void)
{
    return comm_telemetry_debug_stream_mode_name(comm_telemetry_stream_mode);
}

void comm_telemetry_debug_handle_stream(char *mode, comm_telemetry_debug_write_line_t write_line)
{
    DriveEncoderTotal total;

    if(mode == 0)
    {
        comm_telemetry_debug_send_line(write_line, "ERR stream missing_mode");
        return;
    }

    if(strcmp(mode, "off") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_OFF;
    }
    else if(strcmp(mode, "imu_acc") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_IMU_ACC;
    }
    else if(strcmp(mode, "imu_gyro") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_IMU_GYRO;
    }
    else if(strcmp(mode, "imu_yawx") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_IMU_YAWX;
    }
    else if(strcmp(mode, "enc5") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_ENC5;
    }
    else if(strcmp(mode, "enc100") == 0)
    {
        if(DriveGetAllEncoderTotal(&total) == DRIVE_STATUS_OK)
        {
            comm_telemetry_enc100_last_total = total;
        }

        comm_telemetry_enc100_delta.wheel1 = 0;
        comm_telemetry_enc100_delta.wheel2 = 0;
        comm_telemetry_enc100_delta.wheel3 = 0;
        comm_telemetry_enc100_delta.wheel4 = 0;
        comm_telemetry_enc100_elapsed_ms = 0;
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_ENC100;
    }
    else if(strcmp(mode, "duty") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_DUTY;
    }
    else if(strcmp(mode, "speed") == 0)
    {
        comm_telemetry_stream_mode = COMM_TELEMETRY_STREAM_SPEED;
    }
    else
    {
        comm_telemetry_debug_send_line(write_line, "ERR stream bad_mode=%s", mode);
        return;
    }

    comm_telemetry_debug_send_line(write_line,
                                   "OK stream %s",
                                   comm_telemetry_debug_stream_mode_name(comm_telemetry_stream_mode));
}

void comm_telemetry_debug_handle_imu(char *sub_command, comm_telemetry_debug_write_line_t write_line)
{
    int32_t duration_ms;

    if(sub_command == 0)
    {
        comm_telemetry_debug_send_line(write_line, "ERR imu missing_command");
        return;
    }

    if(strcmp(sub_command, "zero") == 0)
    {
        Imu660raResetYaw();
        calibration_yawx_zero(&comm_telemetry_cal_yawx, 0);
        comm_telemetry_debug_send_line(write_line, "OK imu zero");
    }
    else if(strcmp(sub_command, "yawx") == 0)
    {
        char *yawx_command = strtok(0, " ");

        if((yawx_command != 0) && (strcmp(yawx_command, "zero") == 0))
        {
            Imu660raResetYaw();
            calibration_yawx_zero(&comm_telemetry_cal_yawx, 0);
            comm_telemetry_debug_send_line(write_line, "OK imu yawx zero");
        }
        else
        {
            comm_telemetry_debug_send_line(write_line, "ERR imu yawx usage");
        }
    }
    else if(strcmp(sub_command, "stat") == 0)
    {
        char *duration_text = strtok(0, " ");

        if(comm_telemetry_debug_parse_i32(duration_text, &duration_ms) != AI_OK)
        {
            comm_telemetry_debug_send_line(write_line, "ERR imu stat bad_ms");
            return;
        }

        if(duration_ms <= 0)
        {
            comm_telemetry_debug_send_line(write_line, "ERR imu stat bad_ms");
            return;
        }

        comm_telemetry_debug_start_imu_stat((uint32_t)duration_ms, write_line);
    }
    else
    {
        comm_telemetry_debug_send_line(write_line, "ERR imu bad_command=%s", sub_command);
    }
}

void comm_telemetry_debug_tick(comm_telemetry_debug_write_line_t write_line)
{
    comm_telemetry_debug_update_enc100();
    comm_telemetry_debug_update_imu_stat(write_line);
    comm_telemetry_debug_send_stream_frame(write_line);
}
