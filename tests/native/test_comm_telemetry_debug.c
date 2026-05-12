#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ai_error.h"
#include "comm_telemetry_debug.h"
#include "drive.h"
#include "imu660ra.h"
#include "motion.h"

typedef struct
{
    char lines[4][220];
    uint32_t line_count;
    uint32_t reset_yaw_calls;
    DriveEncoderTotal encoder_total;
    DriveEncoderDelta encoder_delta;
    Imu660raScaledData imu;
    ai_status_t imu_status;
    mecanum_duty_t duty;
    motion_speed_sample_t speed_sample;
} test_telemetry_stub_t;

static test_telemetry_stub_t stub;

static void reset_stub(void)
{
    memset(&stub, 0, sizeof(stub));
    stub.imu_status = AI_OK;
}

static void reset_lines(void)
{
    uint32_t i;

    for(i = 0; i < 4U; i++)
    {
        stub.lines[i][0] = '\0';
    }

    stub.line_count = 0;
}

static void capture_line(const char *text)
{
    assert(stub.line_count < 4U);
    (void)snprintf(stub.lines[stub.line_count], sizeof(stub.lines[stub.line_count]), "%s", text);
    stub.line_count++;
}

static void run_stream_command(char *line)
{
    char *command = strtok(line, " ");
    assert(command != 0);
    assert(strcmp(command, "stream") == 0);
    comm_telemetry_debug_handle_stream(strtok(0, " "), capture_line);
}

static void run_imu_command(char *line)
{
    char *command = strtok(line, " ");
    assert(command != 0);
    assert(strcmp(command, "imu") == 0);
    comm_telemetry_debug_handle_imu(strtok(0, " "), capture_line);
}

DriveStatus DriveGetAllEncoderTotal(DriveEncoderTotal *total)
{
    *total = stub.encoder_total;
    return DRIVE_STATUS_OK;
}

DriveStatus DriveGetAllEncoderDelta(DriveEncoderDelta *delta)
{
    *delta = stub.encoder_delta;
    return DRIVE_STATUS_OK;
}

ai_status_t Imu660raGetScaled(Imu660raScaledData *data)
{
    if(stub.imu_status != AI_OK)
    {
        return stub.imu_status;
    }

    *data = stub.imu;
    return AI_OK;
}

void Imu660raResetYaw(void)
{
    stub.reset_yaw_calls++;
}

void motion_test_get_duty(mecanum_duty_t *duty)
{
    *duty = stub.duty;
}

void motion_speed_bench_get_sample(motion_speed_sample_t *sample)
{
    *sample = stub.speed_sample;
}

static void test_stream_imu_acc_outputs_scaled_sample(void)
{
    char line[] = "stream imu_acc";

    reset_stub();
    comm_telemetry_debug_init();
    stub.imu.accXG = 1.0f;
    stub.imu.accYG = 0.0f;
    stub.imu.accZG = 0.0f;

    run_stream_command(line);
    assert(strcmp(stub.lines[0], "OK stream imu_acc") == 0);
    assert(strcmp(comm_telemetry_debug_stream_name(), "imu_acc") == 0);

    reset_lines();
    comm_telemetry_debug_tick(capture_line);

    assert(stub.line_count == 1U);
    assert(strcmp(stub.lines[0], "DATA imu_acc ax_mg=1000 ay_mg=0 az_mg=0 norm_mg=1000") == 0);
}

static void test_imu_yawx_zero_resets_yaw(void)
{
    char line[] = "imu yawx zero";

    reset_stub();
    comm_telemetry_debug_init();

    run_imu_command(line);

    assert(stub.reset_yaw_calls == 1U);
    assert(strcmp(stub.lines[0], "OK imu yawx zero") == 0);
}

static void test_imu_stat_reports_one_sample_window(void)
{
    char line[] = "imu stat 1";

    reset_stub();
    comm_telemetry_debug_init();
    stub.imu.accXG = 1.0f;
    stub.imu.accYG = 0.0f;
    stub.imu.accZG = 0.0f;
    stub.imu.gyroXDps = 1.2f;
    stub.imu.gyroYDps = 2.3f;
    stub.imu.gyroZDps = 3.4f;
    stub.imu.yawDeg = 7.0f;

    run_imu_command(line);
    assert(strcmp(stub.lines[0], "OK imu stat started ms=1 stream=off") == 0);

    reset_lines();
    comm_telemetry_debug_tick(capture_line);

    assert(stub.line_count == 1U);
    assert(strcmp(stub.lines[0],
                  "OK imu stat samples=1 acc_norm_mg=1000 gyro_x_x10=12 gyro_y_x10=23 gyro_z_x10=34 yaw_drift_x10=0") == 0);
}

int main(void)
{
    test_stream_imu_acc_outputs_scaled_sample();
    test_imu_yawx_zero_resets_yaw();
    test_imu_stat_reports_one_sample_window();

    puts("PASS comm_telemetry_debug");
    return 0;
}
