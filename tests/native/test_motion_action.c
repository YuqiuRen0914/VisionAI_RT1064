#include <stdio.h>
#include <stdlib.h>

#include "motion_action.h"
#include "vision_protocol.h"

static void expect_status(const char *name, ai_status_t actual, ai_status_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %ld got %ld\n", name, (long)expected, (long)actual);
        exit(1);
    }
}

static void expect_u8(const char *name, uint8_t actual, uint8_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %u got %u\n", name, (unsigned int)expected, (unsigned int)actual);
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

static void expect_positive(const char *name, float actual)
{
    if(actual <= 0.0f)
    {
        printf("FAIL %s: expected positive got %.3f\n", name, (double)actual);
        exit(1);
    }
}

static void expect_negative(const char *name, float actual)
{
    if(actual >= 0.0f)
    {
        printf("FAIL %s: expected negative got %.3f\n", name, (double)actual);
        exit(1);
    }
}

static motion_action_command_t command(uint8_t cmd, uint8_t dir, uint8_t val)
{
    motion_action_command_t result;

    result.cmd = cmd;
    result.dir = dir;
    result.val = val;
    return result;
}

static void rotate_cw_uses_positive_short_horizon_heading_and_wheel_targets(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_command_t command;
    motion_action_feedback_t feedback;
    motion_action_output_t output;

    motion_action_default_config(&config);
    config.rotate_max_speed_mm_s = 120.0f;
    config.rotate_accel_mm_s2 = 6000.0f;
    config.rotate_kp_mm_s_per_deg = 4.0f;

    command.cmd = VISION_PROTOCOL_CMD_ROTATE;
    command.dir = VISION_PROTOCOL_ROTATE_CW;
    command.val = VISION_PROTOCOL_ROTATE_90_VAL;

    expect_status("init", motion_action_init(&controller, &config), AI_OK);
    expect_status("start rotate", motion_action_start(&controller, &command, 12.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.imu_heading_deg = 12.0f;
    feedback.imu_rate_dps = 0.0f;
    feedback.sensors_valid = 1U;

    expect_status("update rotate", motion_action_update(&controller, &feedback, &output), AI_OK);

    expect_u8("state active", output.state, MOTION_ACTION_STATE_ACTIVE);
    expect_near_f32("target heading", output.debug.target_angle_deg, 90.0f, 0.01f);
    expect_near_f32("short heading", output.debug.heading_deg, 0.0f, 0.01f);
    expect_positive("rot contribution", output.debug.rot_mm_s);
    expect_positive("wheel1 rotate target", output.wheel_targets.wheel1);
    expect_negative("wheel2 rotate target", output.wheel_targets.wheel2);
    expect_positive("wheel3 rotate target", output.wheel_targets.wheel3);
    expect_negative("wheel4 rotate target", output.wheel_targets.wheel4);
}

static void rotate_ccw_180_selects_negative_short_horizon_target(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_command_t rotate = command(VISION_PROTOCOL_CMD_ROTATE,
                                             VISION_PROTOCOL_ROTATE_CCW,
                                             VISION_PROTOCOL_ROTATE_180_VAL);
    motion_action_feedback_t feedback;
    motion_action_output_t output;

    motion_action_default_config(&config);
    config.rotate_accel_mm_s2 = 6000.0f;

    expect_status("init ccw", motion_action_init(&controller, &config), AI_OK);
    expect_status("start ccw", motion_action_start(&controller, &rotate, -30.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.imu_heading_deg = -30.0f;
    feedback.sensors_valid = 1U;

    expect_status("update ccw", motion_action_update(&controller, &feedback, &output), AI_OK);

    expect_near_f32("ccw target", output.debug.target_angle_deg, -180.0f, 0.01f);
    expect_negative("ccw rot contribution", output.debug.rot_mm_s);
    expect_negative("ccw wheel1", output.wheel_targets.wheel1);
    expect_positive("ccw wheel2", output.wheel_targets.wheel2);
    expect_negative("ccw wheel3", output.wheel_targets.wheel3);
    expect_positive("ccw wheel4", output.wheel_targets.wheel4);
}

static void rotate_completion_requires_angle_wheel_speed_and_imu_rate_stable(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_command_t rotate = command(VISION_PROTOCOL_CMD_ROTATE,
                                             VISION_PROTOCOL_ROTATE_CW,
                                             VISION_PROTOCOL_ROTATE_90_VAL);
    motion_action_feedback_t feedback;
    motion_action_output_t output;

    motion_action_default_config(&config);
    config.completion_ms = 100U;

    expect_status("init rotate done", motion_action_init(&controller, &config), AI_OK);
    expect_status("start rotate done", motion_action_start(&controller, &rotate, 0.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 50U;
    feedback.imu_heading_deg = 89.0f;
    feedback.imu_rate_dps = 8.0f;
    feedback.sensors_valid = 1U;

    expect_status("high rate update", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("high rate still active", output.state, MOTION_ACTION_STATE_ACTIVE);
    expect_near_f32("high rate stable time", output.debug.completion_elapsed_ms, 0.0f, 0.01f);

    feedback.imu_rate_dps = 0.0f;
    expect_status("first stable update", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("first stable still active", output.state, MOTION_ACTION_STATE_ACTIVE);
    expect_near_f32("first stable window", (float)output.debug.completion_elapsed_ms, 50.0f, 0.01f);

    expect_status("second stable update", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("second stable done", output.state, MOTION_ACTION_STATE_DONE);
    expect_near_f32("done wheel1 zero", output.wheel_targets.wheel1, 0.0f, 0.01f);
    expect_near_f32("done wheel2 zero", output.wheel_targets.wheel2, 0.0f, 0.01f);
}

static void rotate_fault_attribution_prefers_invalid_sensor_before_motor_or_timeout(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_command_t rotate = command(VISION_PROTOCOL_CMD_ROTATE,
                                             VISION_PROTOCOL_ROTATE_CW,
                                             VISION_PROTOCOL_ROTATE_90_VAL);
    motion_action_feedback_t feedback;
    motion_action_output_t output;

    motion_action_default_config(&config);
    config.timeout_ms = 20U;

    expect_status("init rotate fault", motion_action_init(&controller, &config), AI_OK);
    expect_status("start rotate fault", motion_action_start(&controller, &rotate, 0.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.sensors_valid = 0U;
    feedback.motor_fault = 1U;
    feedback.control_unstable = 1U;

    expect_status("fault update", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("fault state", output.state, MOTION_ACTION_STATE_ERROR);
    expect_u8("sensor invalid first", output.error, VISION_PROTOCOL_ERROR_SENSOR_INVALID);
}

static void move_converts_centimeters_and_maps_direction_to_local_axis(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    motion_action_command_t move_down = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_DOWN, 20U);
    motion_action_command_t move_left = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_LEFT, 20U);
    motion_action_command_t move_right = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_RIGHT, 20U);

    motion_action_default_config(&config);
    config.move_accel_mm_s2 = 10000.0f;

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.sensors_valid = 1U;

    expect_status("init move down", motion_action_init(&controller, &config), AI_OK);
    expect_status("start move down", motion_action_start(&controller, &move_down, 0.0f), AI_OK);
    expect_status("update move down", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_near_f32("down target mm", output.debug.target_distance_mm, -200.0f, 0.01f);
    expect_negative("down vx", output.debug.vx_mm_s);
    motion_action_stop(&controller);

    expect_status("start move left", motion_action_start(&controller, &move_left, 0.0f), AI_OK);
    expect_status("update move left", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_near_f32("left target mm", output.debug.target_distance_mm, 200.0f, 0.01f);
    expect_positive("left vy", output.debug.vy_mm_s);
    motion_action_stop(&controller);

    expect_status("start move right", motion_action_start(&controller, &move_right, 0.0f), AI_OK);
    expect_status("update move right", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_near_f32("right target mm", output.debug.target_distance_mm, -200.0f, 0.01f);
    expect_negative("right vy", output.debug.vy_mm_s);
}

static void move_odometry_projects_wheel_travel_into_local_x_and_y(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    motion_action_command_t move_left = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_LEFT, 20U);

    motion_action_default_config(&config);
    config.wheel_diameter_mm = 100.0f;
    config.counts_per_rev_x100[0] = 314159;
    config.counts_per_rev_x100[1] = 314159;
    config.counts_per_rev_x100[2] = 314159;
    config.counts_per_rev_x100[3] = 314159;
    config.move_accel_mm_s2 = 10000.0f;

    expect_status("init move odom", motion_action_init(&controller, &config), AI_OK);
    expect_status("start move odom", motion_action_start(&controller, &move_left, 0.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.sensors_valid = 1U;
    feedback.encoder_delta.wheel1 = 1000;
    feedback.encoder_delta.wheel2 = -1000;
    feedback.encoder_delta.wheel3 = -1000;
    feedback.encoder_delta.wheel4 = 1000;

    expect_status("update move odom", motion_action_update(&controller, &feedback, &output), AI_OK);

    expect_near_f32("odom x unchanged", output.debug.x_mm, 0.0f, 0.2f);
    expect_near_f32("odom y left", output.debug.y_mm, 100.0f, 0.2f);
    expect_near_f32("left remaining", output.debug.axis_error_mm, 100.0f, 0.2f);
}

static void move_heading_hold_outputs_bounded_rotational_wheel_contribution(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    motion_action_command_t move_up = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 30U);

    motion_action_default_config(&config);
    config.move_accel_mm_s2 = 10000.0f;
    config.heading_hold_kp_mm_s_per_deg = 5.0f;
    config.heading_hold_max_rot_mm_s = 15.0f;

    expect_status("init heading hold", motion_action_init(&controller, &config), AI_OK);
    expect_status("start heading hold", motion_action_start(&controller, &move_up, 10.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.imu_heading_deg = 15.0f;
    feedback.sensors_valid = 1U;

    expect_status("update heading hold", motion_action_update(&controller, &feedback, &output), AI_OK);

    expect_near_f32("heading drift", output.debug.heading_deg, 5.0f, 0.01f);
    expect_near_f32("bounded heading correction", output.debug.rot_mm_s, -15.0f, 0.01f);
    expect_positive("move wheel1 still forward enough", output.wheel_targets.wheel1);
    expect_positive("move wheel2 still forward enough", output.wheel_targets.wheel2);
}

static void move_completion_requires_position_and_wheel_speed_stable(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    motion_action_command_t move_up = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U);

    motion_action_default_config(&config);
    config.wheel_diameter_mm = 100.0f;
    config.counts_per_rev_x100[0] = 314159;
    config.counts_per_rev_x100[1] = 314159;
    config.counts_per_rev_x100[2] = 314159;
    config.counts_per_rev_x100[3] = 314159;
    config.completion_ms = 100U;

    expect_status("init move done", motion_action_init(&controller, &config), AI_OK);
    expect_status("start move done", motion_action_start(&controller, &move_up, 0.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 50U;
    feedback.sensors_valid = 1U;
    feedback.encoder_delta.wheel1 = 950;
    feedback.encoder_delta.wheel2 = 950;
    feedback.encoder_delta.wheel3 = 950;
    feedback.encoder_delta.wheel4 = 950;

    expect_status("first move stable", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("first move stable active", output.state, MOTION_ACTION_STATE_ACTIVE);
    expect_near_f32("move stable window", (float)output.debug.completion_elapsed_ms, 50.0f, 0.01f);

    feedback.encoder_delta.wheel1 = 0;
    feedback.encoder_delta.wheel2 = 0;
    feedback.encoder_delta.wheel3 = 0;
    feedback.encoder_delta.wheel4 = 0;
    expect_status("second move stable", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("second move done", output.state, MOTION_ACTION_STATE_DONE);
    expect_near_f32("done move wheel target", output.wheel_targets.wheel1, 0.0f, 0.01f);
}

static void move_obstruction_is_reported_before_timeout_or_unstable_control(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_feedback_t feedback;
    motion_action_output_t output;
    motion_action_command_t move_up = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 50U);

    motion_action_default_config(&config);
    config.move_accel_mm_s2 = 10000.0f;
    config.obstruction_command_mm_s = 1.0f;
    config.obstruction_ms = 40U;
    config.timeout_ms = 40U;

    expect_status("init obstruct", motion_action_init(&controller, &config), AI_OK);
    expect_status("start obstruct", motion_action_start(&controller, &move_up, 0.0f), AI_OK);

    motion_action_zero_feedback(&feedback);
    feedback.dt_ms = 20U;
    feedback.sensors_valid = 1U;

    expect_status("first obstruct", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("first obstruct active", output.state, MOTION_ACTION_STATE_ACTIVE);

    feedback.control_unstable = 1U;
    expect_status("second obstruct", motion_action_update(&controller, &feedback, &output), AI_OK);
    expect_u8("obstruct state", output.state, MOTION_ACTION_STATE_ERROR);
    expect_u8("obstruct before timeout", output.error, VISION_PROTOCOL_ERROR_OBSTRUCTED);
}

static void action_start_rejects_invalid_move_and_rotate_values(void)
{
    motion_action_controller_t controller;
    motion_action_config_t config;
    motion_action_command_t zero_move = command(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 0U);
    motion_action_command_t bad_rotate = command(VISION_PROTOCOL_CMD_ROTATE, VISION_PROTOCOL_ROTATE_CW, 45U);

    motion_action_default_config(&config);

    expect_status("init validation", motion_action_init(&controller, &config), AI_OK);
    expect_status("zero move rejected", motion_action_start(&controller, &zero_move, 0.0f), AI_ERR_INVALID_ARG);
    expect_status("bad rotate rejected", motion_action_start(&controller, &bad_rotate, 0.0f), AI_ERR_INVALID_ARG);
}

int main(void)
{
    rotate_cw_uses_positive_short_horizon_heading_and_wheel_targets();
    rotate_ccw_180_selects_negative_short_horizon_target();
    rotate_completion_requires_angle_wheel_speed_and_imu_rate_stable();
    rotate_fault_attribution_prefers_invalid_sensor_before_motor_or_timeout();
    move_converts_centimeters_and_maps_direction_to_local_axis();
    move_odometry_projects_wheel_travel_into_local_x_and_y();
    move_heading_hold_outputs_bounded_rotational_wheel_contribution();
    move_completion_requires_position_and_wheel_speed_stable();
    move_obstruction_is_reported_before_timeout_or_unstable_control();
    action_start_rejects_invalid_move_and_rotate_values();
    printf("PASS motion_action\n");
    return 0;
}
