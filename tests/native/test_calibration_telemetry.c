#include <stdio.h>
#include <stdlib.h>

#include "calibration/calibration_telemetry.h"

static void expect_status(const char *name, ai_status_t actual, ai_status_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %ld got %ld\n", name, (long)expected, (long)actual);
        exit(1);
    }
}

static void expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %ld got %ld\n", name, (long)expected, (long)actual);
        exit(1);
    }
}

static void encoder_counts_per_rev_uses_reset_baseline(void)
{
    calibration_encoder_state_t state;
    calibration_wheel_counts_t baseline = {100, -200, 300, -400};
    calibration_wheel_counts_t total = {350, -200, 300, -400};
    calibration_encoder_turn_result_t result;

    calibration_encoder_reset_baseline(&state, &baseline);
    expect_status("counts_per_rev status",
                  calibration_encoder_counts_per_rev(&state, &total, 1U, 2U, &result),
                  AI_OK);
    expect_i32("counts", result.counts, 250);
    expect_i32("turns", (int32_t)result.turns, 2);
    expect_i32("counts_per_rev_x100", result.counts_per_rev_x100, 12500);
}

static void yawx_sample_uses_zero_baseline(void)
{
    calibration_yawx_state_t state;
    calibration_yawx_sample_t sample;

    calibration_yawx_zero(&state, 120);
    expect_status("yawx sample status",
                  calibration_yawx_sample(&state, 35, 155, &sample),
                  AI_OK);
    expect_i32("gx_x10", sample.gx_x10, 35);
    expect_i32("yawx_x10", sample.yawx_x10, 35);
}

static void encoder_delta_reports_all_wheels_from_baseline(void)
{
    calibration_encoder_state_t state;
    calibration_wheel_counts_t baseline = {10, 20, 30, 40};
    calibration_wheel_counts_t total = {15, 5, 70, -10};
    calibration_wheel_counts_t delta;

    calibration_encoder_reset_baseline(&state, &baseline);
    expect_status("delta status", calibration_encoder_delta(&state, &total, &delta), AI_OK);
    expect_i32("delta wheel1", delta.wheel1, 5);
    expect_i32("delta wheel2", delta.wheel2, -15);
    expect_i32("delta wheel3", delta.wheel3, 40);
    expect_i32("delta wheel4", delta.wheel4, -50);
}

static void encoder_counts_per_rev_rejects_bad_inputs(void)
{
    calibration_encoder_state_t state;
    calibration_wheel_counts_t total = {0, 0, 0, 0};
    calibration_encoder_turn_result_t result;

    calibration_encoder_reset_baseline(&state, &total);
    expect_status("bad wheel low",
                  calibration_encoder_counts_per_rev(&state, &total, 0U, 1U, &result),
                  AI_ERR_INVALID_ARG);
    expect_status("bad wheel high",
                  calibration_encoder_counts_per_rev(&state, &total, 5U, 1U, &result),
                  AI_ERR_INVALID_ARG);
    expect_status("zero turns",
                  calibration_encoder_counts_per_rev(&state, &total, 1U, 0U, &result),
                  AI_ERR_INVALID_ARG);
}

int main(void)
{
    encoder_counts_per_rev_uses_reset_baseline();
    yawx_sample_uses_zero_baseline();
    encoder_delta_reports_all_wheels_from_baseline();
    encoder_counts_per_rev_rejects_bad_inputs();
    printf("PASS calibration_telemetry\n");
    return 0;
}
