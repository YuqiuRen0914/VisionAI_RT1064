#include "motion_action.h"

#include "vision_protocol.h"

#define MOTION_ACTION_PI_F (3.14159265358979323846f)

static float motion_action_abs_f32(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float motion_action_sign_f32(float value)
{
    if(value > 0.0f)
    {
        return 1.0f;
    }

    if(value < 0.0f)
    {
        return -1.0f;
    }

    return 0.0f;
}

static float motion_action_clamp_f32(float value, float limit)
{
    if(limit < 0.0f)
    {
        limit = -limit;
    }

    if(value > limit)
    {
        return limit;
    }

    if(value < -limit)
    {
        return -limit;
    }

    return value;
}

static uint8_t motion_action_move_valid(const motion_action_command_t *command)
{
    if(command->val == 0U)
    {
        return 0U;
    }

    return (command->dir <= VISION_PROTOCOL_MOVE_RIGHT) ? 1U : 0U;
}

static uint8_t motion_action_rotate_valid(const motion_action_command_t *command)
{
    if(command->dir > VISION_PROTOCOL_ROTATE_CW)
    {
        return 0U;
    }

    return ((command->val == VISION_PROTOCOL_ROTATE_90_VAL) ||
            (command->val == VISION_PROTOCOL_ROTATE_180_VAL)) ? 1U : 0U;
}

static ai_status_t motion_action_validate_config(const motion_action_config_t *config)
{
    if(config == 0)
    {
        return AI_ERR_INVALID_ARG;
    }

    if((config->wheel_diameter_mm <= 0.0f) ||
       (config->move_max_speed_mm_s <= 0.0f) ||
       (config->move_accel_mm_s2 <= 0.0f) ||
       (config->rotate_max_speed_mm_s <= 0.0f) ||
       (config->rotate_accel_mm_s2 <= 0.0f) ||
       (config->completion_ms == 0U) ||
       (config->timeout_ms == 0U) ||
       (config->odom_x_sign == 0) ||
       (config->odom_y_sign == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        if(config->counts_per_rev_x100[i] <= 0)
        {
            return AI_ERR_INVALID_ARG;
        }
    }

    return AI_OK;
}

static float motion_action_get_delta(const motion_speed_encoder_delta_t *delta, uint8_t index)
{
    if(index == 0U)
    {
        return (float)delta->wheel1;
    }

    if(index == 1U)
    {
        return (float)delta->wheel2;
    }

    if(index == 2U)
    {
        return (float)delta->wheel3;
    }

    return (float)delta->wheel4;
}

static float motion_action_max_wheel_abs(const motion_speed_wheel_float_t *value)
{
    float max_abs = motion_action_abs_f32(value->wheel1);

    if(motion_action_abs_f32(value->wheel2) > max_abs)
    {
        max_abs = motion_action_abs_f32(value->wheel2);
    }

    if(motion_action_abs_f32(value->wheel3) > max_abs)
    {
        max_abs = motion_action_abs_f32(value->wheel3);
    }

    if(motion_action_abs_f32(value->wheel4) > max_abs)
    {
        max_abs = motion_action_abs_f32(value->wheel4);
    }

    return max_abs;
}

static float motion_action_counts_to_mm(const motion_action_config_t *config,
                                        uint8_t wheel_index,
                                        float counts)
{
    const float circumference_mm = config->wheel_diameter_mm * MOTION_ACTION_PI_F;

    return (counts * circumference_mm * 100.0f) /
           (float)config->counts_per_rev_x100[wheel_index];
}

static void motion_action_zero_wheels(motion_speed_wheel_float_t *wheels)
{
    wheels->wheel1 = 0.0f;
    wheels->wheel2 = 0.0f;
    wheels->wheel3 = 0.0f;
    wheels->wheel4 = 0.0f;
}

static void motion_action_solve_wheels(float vx_mm_s,
                                       float vy_mm_s,
                                       float rot_mm_s,
                                       motion_speed_wheel_float_t *targets)
{
    targets->wheel1 = vx_mm_s + vy_mm_s + rot_mm_s;
    targets->wheel2 = vx_mm_s - vy_mm_s - rot_mm_s;
    targets->wheel3 = vx_mm_s - vy_mm_s + rot_mm_s;
    targets->wheel4 = vx_mm_s + vy_mm_s - rot_mm_s;
}

static void motion_action_limit_wheels(motion_speed_wheel_float_t *targets, float limit)
{
    const float max_abs = motion_action_max_wheel_abs(targets);

    if((limit <= 0.0f) || (max_abs <= limit))
    {
        return;
    }

    targets->wheel1 = (targets->wheel1 * limit) / max_abs;
    targets->wheel2 = (targets->wheel2 * limit) / max_abs;
    targets->wheel3 = (targets->wheel3 * limit) / max_abs;
    targets->wheel4 = (targets->wheel4 * limit) / max_abs;
}

static float motion_action_apply_accel(float previous,
                                       float desired,
                                       float accel_mm_s2,
                                       uint32_t dt_ms)
{
    const float max_step = accel_mm_s2 * ((float)dt_ms / 1000.0f);
    const float delta = desired - previous;

    if(delta > max_step)
    {
        return previous + max_step;
    }

    if(delta < -max_step)
    {
        return previous - max_step;
    }

    return desired;
}

static float motion_action_profile_speed(float error,
                                         float threshold,
                                         float max_speed,
                                         float approach_speed,
                                         float kp)
{
    float speed_abs;

    if(motion_action_abs_f32(error) <= threshold)
    {
        return 0.0f;
    }

    speed_abs = motion_action_abs_f32(error) * kp;

    if(speed_abs > max_speed)
    {
        speed_abs = max_speed;
    }

    if((approach_speed > 0.0f) && (speed_abs < approach_speed))
    {
        speed_abs = approach_speed;
    }

    return motion_action_sign_f32(error) * speed_abs;
}

static uint8_t motion_action_wheels_stopped(const motion_action_config_t *config,
                                            const motion_speed_wheel_float_t *measured)
{
    return (motion_action_max_wheel_abs(measured) <= config->wheel_stop_mm_s) ? 1U : 0U;
}

static void motion_action_update_odometry(motion_action_controller_t *controller,
                                          const motion_action_feedback_t *feedback)
{
    float wheel_mm[MOTION_SPEED_WHEEL_COUNT];
    float dx_mm;
    float dy_mm;

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        wheel_mm[i] = motion_action_counts_to_mm(&controller->config,
                                                 i,
                                                 motion_action_get_delta(&feedback->encoder_delta, i));
    }

    dx_mm = (wheel_mm[0] + wheel_mm[1] + wheel_mm[2] + wheel_mm[3]) * 0.25f;
    dy_mm = (wheel_mm[0] - wheel_mm[1] - wheel_mm[2] + wheel_mm[3]) * 0.25f;

    controller->x_mm += dx_mm * (float)controller->config.odom_x_sign;
    controller->y_mm += dy_mm * (float)controller->config.odom_y_sign;
}

static void motion_action_refresh_debug(motion_action_controller_t *controller,
                                        const motion_action_feedback_t *feedback,
                                        float axis_error_mm,
                                        float heading_deg,
                                        float heading_error_deg,
                                        float vx_mm_s,
                                        float vy_mm_s,
                                        float rot_mm_s)
{
    controller->debug.cmd = controller->command.cmd;
    controller->debug.dir = controller->command.dir;
    controller->debug.val = controller->command.val;
    controller->debug.target_distance_mm = controller->target_distance_mm;
    controller->debug.x_mm = controller->x_mm;
    controller->debug.y_mm = controller->y_mm;
    controller->debug.axis_error_mm = axis_error_mm;
    controller->debug.target_angle_deg = controller->target_angle_deg;
    controller->debug.heading_deg = heading_deg;
    controller->debug.heading_error_deg = heading_error_deg;
    controller->debug.imu_rate_dps = feedback->imu_rate_dps;
    controller->debug.vx_mm_s = vx_mm_s;
    controller->debug.vy_mm_s = vy_mm_s;
    controller->debug.rot_mm_s = rot_mm_s;
    controller->debug.elapsed_ms = controller->elapsed_ms;
    controller->debug.completion_elapsed_ms = controller->completion_elapsed_ms;
    controller->debug.blocked_ms = controller->blocked_ms;
}

static void motion_action_finish(motion_action_controller_t *controller,
                                 motion_action_output_t *output,
                                 uint8_t state,
                                 uint8_t error,
                                 uint8_t context)
{
    controller->state = state;
    controller->axis_speed_mm_s = 0.0f;
    controller->rot_mm_s = 0.0f;
    output->state = state;
    output->error = error;
    output->context = context;
    output->elapsed_ms = controller->elapsed_ms;
    motion_action_zero_wheels(&output->wheel_targets);
    output->debug = controller->debug;
}

static void motion_action_fail(motion_action_controller_t *controller,
                               motion_action_output_t *output,
                               uint8_t error,
                               uint8_t context)
{
    motion_action_finish(controller, output, MOTION_ACTION_STATE_ERROR, error, context);
}

static uint8_t motion_action_is_x_move(uint8_t dir)
{
    return ((dir == VISION_PROTOCOL_MOVE_UP) || (dir == VISION_PROTOCOL_MOVE_DOWN)) ? 1U : 0U;
}

static float motion_action_move_axis_position(const motion_action_controller_t *controller)
{
    if(motion_action_is_x_move(controller->command.dir) != 0U)
    {
        return controller->x_mm;
    }

    return controller->y_mm;
}

void motion_action_default_config(motion_action_config_t *config)
{
    if(config == 0)
    {
        return;
    }

    config->wheel_diameter_mm = 63.0f;

    for(uint8_t i = 0; i < MOTION_SPEED_WHEEL_COUNT; i++)
    {
        config->counts_per_rev_x100[i] = 239000;
    }

    config->move_max_speed_mm_s = 180.0f;
    config->move_accel_mm_s2 = 600.0f;
    config->move_kp_mm_s_per_mm = 3.0f;
    config->move_approach_speed_mm_s = 40.0f;
    config->rotate_max_speed_mm_s = 120.0f;
    config->rotate_accel_mm_s2 = 400.0f;
    config->rotate_kp_mm_s_per_deg = 4.0f;
    config->rotate_approach_speed_mm_s = 30.0f;
    config->heading_hold_kp_mm_s_per_deg = 3.0f;
    config->heading_hold_max_rot_mm_s = 80.0f;
    config->move_done_error_mm = 10.0f;
    config->rotate_done_error_deg = 2.0f;
    config->wheel_stop_mm_s = 30.0f;
    config->imu_stop_dps = 5.0f;
    config->completion_ms = 100U;
    config->timeout_ms = 8000U;
    config->obstruction_ms = 500U;
    config->obstruction_command_mm_s = 40.0f;
    config->odom_x_sign = 1;
    config->odom_y_sign = 1;
}

ai_status_t motion_action_init(motion_action_controller_t *controller,
                               const motion_action_config_t *config)
{
    if((controller == 0) || (motion_action_validate_config(config) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    controller->config = *config;
    motion_action_stop(controller);
    return AI_OK;
}

ai_status_t motion_action_start(motion_action_controller_t *controller,
                                const motion_action_command_t *command,
                                float start_heading_deg)
{
    motion_action_feedback_t start_feedback;

    if((controller == 0) || (command == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(controller->state == MOTION_ACTION_STATE_ACTIVE)
    {
        return AI_ERR_BUSY;
    }

    if(command->cmd == VISION_PROTOCOL_CMD_MOVE)
    {
        if(motion_action_move_valid(command) == 0U)
        {
            return AI_ERR_INVALID_ARG;
        }
    }
    else if(command->cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        if(motion_action_rotate_valid(command) == 0U)
        {
            return AI_ERR_INVALID_ARG;
        }
    }
    else
    {
        return AI_ERR_INVALID_ARG;
    }

    controller->state = MOTION_ACTION_STATE_ACTIVE;
    controller->command = *command;
    controller->heading_baseline_deg = start_heading_deg;
    controller->target_distance_mm = 0.0f;
    controller->target_angle_deg = 0.0f;
    controller->x_mm = 0.0f;
    controller->y_mm = 0.0f;
    controller->axis_speed_mm_s = 0.0f;
    controller->rot_mm_s = 0.0f;
    controller->elapsed_ms = 0U;
    controller->completion_elapsed_ms = 0U;
    controller->blocked_ms = 0U;

    if(command->cmd == VISION_PROTOCOL_CMD_MOVE)
    {
        controller->target_distance_mm = (float)command->val * 10.0f;

        if((command->dir == VISION_PROTOCOL_MOVE_DOWN) ||
           (command->dir == VISION_PROTOCOL_MOVE_RIGHT))
        {
            controller->target_distance_mm = -controller->target_distance_mm;
        }
    }
    else
    {
        controller->target_angle_deg = (float)command->val;

        if(command->dir == VISION_PROTOCOL_ROTATE_CCW)
        {
            controller->target_angle_deg = -controller->target_angle_deg;
        }
    }

    motion_action_zero_feedback(&start_feedback);
    start_feedback.imu_heading_deg = start_heading_deg;
    start_feedback.sensors_valid = 1U;

    motion_action_refresh_debug(controller,
                                &start_feedback,
                                controller->target_distance_mm,
                                0.0f,
                                controller->target_angle_deg,
                                0.0f,
                                0.0f,
                                0.0f);
    return AI_OK;
}

ai_status_t motion_action_update(motion_action_controller_t *controller,
                                 const motion_action_feedback_t *feedback,
                                 motion_action_output_t *output)
{
    motion_speed_wheel_float_t targets;
    float heading_deg;
    float heading_error_deg;
    float axis_position_mm;
    float axis_error_mm;
    float vx_mm_s = 0.0f;
    float vy_mm_s = 0.0f;
    float rot_mm_s = 0.0f;
    uint8_t complete = 0U;

    if((controller == 0) || (feedback == 0) || (output == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    motion_action_zero_output(output);

    if(controller->state != MOTION_ACTION_STATE_ACTIVE)
    {
        output->state = controller->state;
        output->debug = controller->debug;
        return AI_ERR_NO_DATA;
    }

    if(feedback->dt_ms == 0U)
    {
        output->state = controller->state;
        output->debug = controller->debug;
        return AI_ERR_NO_DATA;
    }

    if(controller->elapsed_ms <= (UINT32_MAX - feedback->dt_ms))
    {
        controller->elapsed_ms += feedback->dt_ms;
    }
    else
    {
        controller->elapsed_ms = UINT32_MAX;
    }

    if(feedback->sensors_valid == 0U)
    {
        motion_action_fail(controller, output, VISION_PROTOCOL_ERROR_SENSOR_INVALID, controller->command.cmd);
        return AI_OK;
    }

    if(feedback->motor_fault != 0U)
    {
        motion_action_fail(controller, output, VISION_PROTOCOL_ERROR_MOTOR_FAULT, controller->command.cmd);
        return AI_OK;
    }

    motion_action_update_odometry(controller, feedback);

    heading_deg = feedback->imu_heading_deg - controller->heading_baseline_deg;
    heading_error_deg = controller->target_angle_deg - heading_deg;
    axis_position_mm = motion_action_move_axis_position(controller);
    axis_error_mm = controller->target_distance_mm - axis_position_mm;

    if(controller->command.cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        rot_mm_s = motion_action_profile_speed(heading_error_deg,
                                               controller->config.rotate_done_error_deg,
                                               controller->config.rotate_max_speed_mm_s,
                                               controller->config.rotate_approach_speed_mm_s,
                                               controller->config.rotate_kp_mm_s_per_deg);
        rot_mm_s = motion_action_apply_accel(controller->rot_mm_s,
                                             rot_mm_s,
                                             controller->config.rotate_accel_mm_s2,
                                             feedback->dt_ms);
        controller->rot_mm_s = rot_mm_s;

        if((motion_action_abs_f32(heading_error_deg) <= controller->config.rotate_done_error_deg) &&
           (motion_action_wheels_stopped(&controller->config, &feedback->measured_mm_s) != 0U) &&
           (motion_action_abs_f32(feedback->imu_rate_dps) <= controller->config.imu_stop_dps))
        {
            complete = 1U;
        }
    }
    else
    {
        const float desired_axis_speed = motion_action_profile_speed(axis_error_mm,
                                                                     controller->config.move_done_error_mm,
                                                                     controller->config.move_max_speed_mm_s,
                                                                     controller->config.move_approach_speed_mm_s,
                                                                     controller->config.move_kp_mm_s_per_mm);
        const float desired_heading_rot = motion_action_clamp_f32((0.0f - heading_deg) *
                                                                  controller->config.heading_hold_kp_mm_s_per_deg,
                                                                  controller->config.heading_hold_max_rot_mm_s);

        controller->axis_speed_mm_s = motion_action_apply_accel(controller->axis_speed_mm_s,
                                                                desired_axis_speed,
                                                                controller->config.move_accel_mm_s2,
                                                                feedback->dt_ms);
        controller->rot_mm_s = desired_heading_rot;

        if(motion_action_is_x_move(controller->command.dir) != 0U)
        {
            vx_mm_s = controller->axis_speed_mm_s;
        }
        else
        {
            vy_mm_s = controller->axis_speed_mm_s;
        }

        rot_mm_s = controller->rot_mm_s;

        if((motion_action_abs_f32(axis_error_mm) <= controller->config.move_done_error_mm) &&
           (motion_action_wheels_stopped(&controller->config, &feedback->measured_mm_s) != 0U))
        {
            complete = 1U;
        }
    }

    if(complete != 0U)
    {
        if(controller->completion_elapsed_ms <= (UINT32_MAX - feedback->dt_ms))
        {
            controller->completion_elapsed_ms += feedback->dt_ms;
        }
        else
        {
            controller->completion_elapsed_ms = UINT32_MAX;
        }
    }
    else
    {
        controller->completion_elapsed_ms = 0U;
    }

    motion_action_refresh_debug(controller,
                                feedback,
                                axis_error_mm,
                                heading_deg,
                                heading_error_deg,
                                vx_mm_s,
                                vy_mm_s,
                                rot_mm_s);

    if(controller->completion_elapsed_ms >= controller->config.completion_ms)
    {
        motion_action_finish(controller, output, MOTION_ACTION_STATE_DONE, 0U, 0U);
        return AI_OK;
    }

    if((motion_action_abs_f32(vx_mm_s) >= controller->config.obstruction_command_mm_s) ||
       (motion_action_abs_f32(vy_mm_s) >= controller->config.obstruction_command_mm_s) ||
       (motion_action_abs_f32(rot_mm_s) >= controller->config.obstruction_command_mm_s))
    {
        if((complete == 0U) &&
           (motion_action_wheels_stopped(&controller->config, &feedback->measured_mm_s) != 0U))
        {
            if(controller->blocked_ms <= (UINT32_MAX - feedback->dt_ms))
            {
                controller->blocked_ms += feedback->dt_ms;
            }
            else
            {
                controller->blocked_ms = UINT32_MAX;
            }
        }
        else
        {
            controller->blocked_ms = 0U;
        }
    }
    else
    {
        controller->blocked_ms = 0U;
    }

    if(controller->blocked_ms >= controller->config.obstruction_ms)
    {
        motion_action_refresh_debug(controller,
                                    feedback,
                                    axis_error_mm,
                                    heading_deg,
                                    heading_error_deg,
                                    vx_mm_s,
                                    vy_mm_s,
                                    rot_mm_s);
        motion_action_fail(controller, output, VISION_PROTOCOL_ERROR_OBSTRUCTED, controller->command.cmd);
        return AI_OK;
    }

    if(controller->elapsed_ms >= controller->config.timeout_ms)
    {
        motion_action_fail(controller, output, VISION_PROTOCOL_ERROR_TIMEOUT, controller->command.cmd);
        return AI_OK;
    }

    if(feedback->control_unstable != 0U)
    {
        motion_action_fail(controller, output, VISION_PROTOCOL_ERROR_CONTROL_UNSTABLE, controller->command.cmd);
        return AI_OK;
    }

    motion_action_solve_wheels(vx_mm_s, vy_mm_s, rot_mm_s, &targets);

    if(controller->command.cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        motion_action_limit_wheels(&targets, controller->config.rotate_max_speed_mm_s);
    }
    else
    {
        motion_action_limit_wheels(&targets, controller->config.move_max_speed_mm_s);
    }

    output->state = MOTION_ACTION_STATE_ACTIVE;
    output->elapsed_ms = controller->elapsed_ms;
    output->wheel_targets = targets;
    output->debug = controller->debug;

    return AI_OK;
}

void motion_action_stop(motion_action_controller_t *controller)
{
    if(controller == 0)
    {
        return;
    }

    controller->state = MOTION_ACTION_STATE_IDLE;
    controller->command.cmd = 0;
    controller->command.dir = 0;
    controller->command.val = 0;
    controller->heading_baseline_deg = 0.0f;
    controller->target_distance_mm = 0.0f;
    controller->target_angle_deg = 0.0f;
    controller->x_mm = 0.0f;
    controller->y_mm = 0.0f;
    controller->axis_speed_mm_s = 0.0f;
    controller->rot_mm_s = 0.0f;
    controller->elapsed_ms = 0U;
    controller->completion_elapsed_ms = 0U;
    controller->blocked_ms = 0U;
    controller->debug.cmd = 0;
    controller->debug.dir = 0;
    controller->debug.val = 0;
    controller->debug.target_distance_mm = 0.0f;
    controller->debug.x_mm = 0.0f;
    controller->debug.y_mm = 0.0f;
    controller->debug.axis_error_mm = 0.0f;
    controller->debug.target_angle_deg = 0.0f;
    controller->debug.heading_deg = 0.0f;
    controller->debug.heading_error_deg = 0.0f;
    controller->debug.imu_rate_dps = 0.0f;
    controller->debug.vx_mm_s = 0.0f;
    controller->debug.vy_mm_s = 0.0f;
    controller->debug.rot_mm_s = 0.0f;
    controller->debug.elapsed_ms = 0U;
    controller->debug.completion_elapsed_ms = 0U;
    controller->debug.blocked_ms = 0U;
}

void motion_action_zero_feedback(motion_action_feedback_t *feedback)
{
    if(feedback == 0)
    {
        return;
    }

    feedback->encoder_delta.wheel1 = 0;
    feedback->encoder_delta.wheel2 = 0;
    feedback->encoder_delta.wheel3 = 0;
    feedback->encoder_delta.wheel4 = 0;
    motion_action_zero_wheels(&feedback->measured_mm_s);
    feedback->imu_heading_deg = 0.0f;
    feedback->imu_rate_dps = 0.0f;
    feedback->dt_ms = 0U;
    feedback->sensors_valid = 0U;
    feedback->motor_fault = 0U;
    feedback->control_unstable = 0U;
}

void motion_action_zero_output(motion_action_output_t *output)
{
    if(output == 0)
    {
        return;
    }

    output->state = MOTION_ACTION_STATE_IDLE;
    output->error = 0U;
    output->context = 0U;
    output->elapsed_ms = 0U;
    motion_action_zero_wheels(&output->wheel_targets);
    output->debug.cmd = 0;
    output->debug.dir = 0;
    output->debug.val = 0;
    output->debug.target_distance_mm = 0.0f;
    output->debug.x_mm = 0.0f;
    output->debug.y_mm = 0.0f;
    output->debug.axis_error_mm = 0.0f;
    output->debug.target_angle_deg = 0.0f;
    output->debug.heading_deg = 0.0f;
    output->debug.heading_error_deg = 0.0f;
    output->debug.imu_rate_dps = 0.0f;
    output->debug.vx_mm_s = 0.0f;
    output->debug.vy_mm_s = 0.0f;
    output->debug.rot_mm_s = 0.0f;
    output->debug.elapsed_ms = 0U;
    output->debug.completion_elapsed_ms = 0U;
    output->debug.blocked_ms = 0U;
}

void motion_action_get_debug(const motion_action_controller_t *controller,
                             motion_action_debug_t *debug)
{
    if((controller == 0) || (debug == 0))
    {
        return;
    }

    *debug = controller->debug;
}

uint8_t motion_action_is_active(const motion_action_controller_t *controller)
{
    if(controller == 0)
    {
        return 0U;
    }

    return (controller->state == MOTION_ACTION_STATE_ACTIVE) ? 1U : 0U;
}
