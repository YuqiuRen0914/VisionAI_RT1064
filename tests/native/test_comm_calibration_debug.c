#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "comm_calibration_debug.h"
#include "drive.h"

typedef struct
{
    char last_line[180];
    DriveEncoderTotal total;
    DriveStatus total_status;
} test_calibration_debug_stub_t;

static test_calibration_debug_stub_t stub;

static void reset_stub(void)
{
    memset(&stub, 0, sizeof(stub));
    stub.total_status = DRIVE_STATUS_OK;
}

static void capture_line(const char *text)
{
    (void)snprintf(stub.last_line, sizeof(stub.last_line), "%s", text);
}

static void run_command(char *line)
{
    char *command = strtok(line, " ");
    assert(command != 0);
    assert(strcmp(command, "cal") == 0);
    comm_calibration_debug_handle(strtok(0, " "), capture_line);
}

DriveStatus DriveGetAllEncoderTotal(DriveEncoderTotal *total)
{
    if(stub.total_status != DRIVE_STATUS_OK)
    {
        return stub.total_status;
    }

    *total = stub.total;
    return DRIVE_STATUS_OK;
}

static void test_total_reports_encoder_counts(void)
{
    char line[] = "cal enc total";

    reset_stub();
    stub.total.wheel1 = 10;
    stub.total.wheel2 = 20;
    stub.total.wheel3 = -30;
    stub.total.wheel4 = 40;
    comm_calibration_debug_init();

    run_command(line);

    assert(strcmp(stub.last_line, "DATA cal_enc_total w1=10 w2=20 w3=-30 w4=40") == 0);
}

static void test_zero_then_delta_uses_new_baseline(void)
{
    char zero_line[] = "cal enc zero";
    char delta_line[] = "cal enc delta";

    reset_stub();
    stub.total.wheel1 = 100;
    stub.total.wheel2 = 200;
    stub.total.wheel3 = 300;
    stub.total.wheel4 = 400;
    comm_calibration_debug_init();

    run_command(zero_line);
    assert(strcmp(stub.last_line, "OK cal enc zero w1=100 w2=200 w3=300 w4=400") == 0);

    stub.total.wheel1 = 125;
    stub.total.wheel2 = 180;
    stub.total.wheel3 = 333;
    stub.total.wheel4 = 400;
    run_command(delta_line);

    assert(strcmp(stub.last_line, "DATA cal_enc_delta w1=25 w2=-20 w3=33 w4=0") == 0);
}

static void test_wheel_reports_fixed_point_counts_per_rev(void)
{
    char line[] = "cal enc wheel 2 3";

    reset_stub();
    stub.total.wheel1 = 0;
    stub.total.wheel2 = 100;
    stub.total.wheel3 = 0;
    stub.total.wheel4 = 0;
    comm_calibration_debug_init();

    stub.total.wheel2 = 250;
    run_command(line);

    assert(strcmp(stub.last_line, "OK cal enc wheel=2 counts=150 turns=3 counts_per_rev_x100=5000") == 0);
}

static void test_bad_wheel_argument_reports_specific_error(void)
{
    char line[] = "cal enc wheel 5 1";

    reset_stub();
    comm_calibration_debug_init();
    run_command(line);

    assert(strcmp(stub.last_line, "ERR cal enc bad_wheel") == 0);
}

int main(void)
{
    test_total_reports_encoder_counts();
    test_zero_then_delta_uses_new_baseline();
    test_wheel_reports_fixed_point_counts_per_rev();
    test_bad_wheel_argument_reports_specific_error();

    puts("PASS comm_calibration_debug");
    return 0;
}
