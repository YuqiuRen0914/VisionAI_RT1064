#include <stdio.h>
#include <stdlib.h>

#include "motion_speed.h"

static void expect_status(const char *name, ai_status_t actual, ai_status_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %ld got %ld\n", name, (long)expected, (long)actual);
        exit(1);
    }
}

static void expect_near_f32(const char *name, float actual, float expected, float tolerance)
{
    float diff = actual - expected;

    if(diff < 0.0f)
    {
        diff = -diff;
    }

    if(diff > tolerance)
    {
        printf("FAIL %s: expected %.3f got %.3f tolerance %.3f\n",
               name,
               (double)expected,
               (double)actual,
               (double)tolerance);
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

static void speed_observation_uses_encoder_total_delta_over_actual_dt(void)
{
    motion_speed_controller_t controller;
    motion_speed_config_t config;
    motion_speed_encoder_total_t total = {0, 0, 0, 0};
    motion_speed_sample_t sample;

    motion_speed_default_config(&config);
    config.wheel_diameter_mm = 100.0f;
    config.counts_per_rev_x100[0] = 20000;
    config.counts_per_rev_x100[1] = 20000;
    config.counts_per_rev_x100[2] = 20000;
    config.counts_per_rev_x100[3] = 20000;
    config.max_speed_mm_s = 200.0f;
    config.duty_limit_percent = 30.0f;

    expect_status("init", motion_speed_init(&controller, &config), AI_OK);
    expect_status("set target",
                  motion_speed_set_targets(&controller, 250.0f, 0.0f, 0.0f, 0.0f),
                  AI_OK);

    expect_status("first sample", motion_speed_update(&controller, &total, 1000U, &sample), AI_ERR_NO_DATA);

    total.wheel1 = 100;
    expect_status("second sample", motion_speed_update(&controller, &total, 1100U, &sample), AI_OK);

    expect_i32("encoder delta wheel1", sample.encoder_delta.wheel1, 100);
    expect_i32("dt_ms", (int32_t)sample.dt_ms, 100);
    expect_near_f32("measured wheel1 mm/s", sample.measured_mm_s.wheel1, 1570.8f, 0.8f);
    expect_near_f32("target wheel1 clamped", sample.target_mm_s.wheel1, 200.0f, 0.01f);
}

static void zero_target_clears_integral_and_duty(void)
{
    motion_speed_controller_t controller;
    motion_speed_config_t config;
    motion_speed_encoder_total_t total = {0, 0, 0, 0};
    motion_speed_sample_t sample;

    motion_speed_default_config(&config);
    config.kp = 0.10f;
    config.ki = 0.20f;
    config.duty_limit_percent = 30.0f;

    expect_status("init", motion_speed_init(&controller, &config), AI_OK);
    expect_status("set target", motion_speed_set_targets(&controller, 100.0f, 0.0f, 0.0f, 0.0f), AI_OK);
    (void)motion_speed_update(&controller, &total, 10U, &sample);
    expect_status("run sample", motion_speed_update(&controller, &total, 20U, &sample), AI_OK);

    if(sample.duty_percent.wheel1 <= 0.0f)
    {
        printf("FAIL duty should be positive before zero target, got %.3f\n", (double)sample.duty_percent.wheel1);
        exit(1);
    }

    expect_status("zero target", motion_speed_set_targets(&controller, 0.0f, 0.0f, 0.0f, 0.0f), AI_OK);
    expect_status("zero sample", motion_speed_update(&controller, &total, 30U, &sample), AI_OK);

    expect_near_f32("zero duty wheel1", sample.duty_percent.wheel1, 0.0f, 0.001f);
    expect_near_f32("zero integral wheel1", motion_speed_get_integral_duty(&controller, 0U), 0.0f, 0.001f);
}

static void saturated_output_does_not_wind_up_integral(void)
{
    motion_speed_controller_t controller;
    motion_speed_config_t config;
    motion_speed_encoder_total_t total = {0, 0, 0, 0};
    motion_speed_sample_t sample;

    motion_speed_default_config(&config);
    config.kp = 1.0f;
    config.ki = 1.0f;
    config.duty_limit_percent = 30.0f;
    config.max_speed_mm_s = 200.0f;

    expect_status("init", motion_speed_init(&controller, &config), AI_OK);
    expect_status("set target", motion_speed_set_targets(&controller, 200.0f, 0.0f, 0.0f, 0.0f), AI_OK);
    (void)motion_speed_update(&controller, &total, 0U, &sample);
    expect_status("run sample", motion_speed_update(&controller, &total, 100U, &sample), AI_OK);

    expect_near_f32("clamped duty", sample.duty_percent.wheel1, 30.0f, 0.001f);
    expect_near_f32("no windup", motion_speed_get_integral_duty(&controller, 0U), 0.0f, 0.001f);
}

int main(void)
{
    speed_observation_uses_encoder_total_delta_over_actual_dt();
    zero_target_clears_integral_and_duty();
    saturated_output_does_not_wind_up_integral();
    printf("PASS motion_speed\n");
    return 0;
}
