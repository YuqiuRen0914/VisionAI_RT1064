#include "motion_defaults.h"

#include "drive_config.h"
#include "motion_config.h"

void motion_defaults_load_speed_config(motion_speed_config_t *config)
{
    if(config == 0)
    {
        return;
    }

    motion_speed_default_config(config);
    config->kp = MOTION_SPEED_KP_DEFAULT;
    config->ki = MOTION_SPEED_KI_DEFAULT;
    config->kd = MOTION_SPEED_KD_DEFAULT;
    config->duty_limit_percent = MOTION_CLOSED_LOOP_DUTY_LIMIT_PERCENT;
    config->max_speed_mm_s = MOTION_CLOSED_LOOP_MAX_SPEED_MM_S;
    config->feedforward_duty_per_mm_s = MOTION_SPEED_FEEDFORWARD_DEFAULT;
    config->static_duty_percent = MOTION_SPEED_STATIC_DUTY_DEFAULT;
    config->static_threshold_mm_s = MOTION_SPEED_STATIC_THRESHOLD_MM_S;
    config->speed_filter_tau_ms = MOTION_SPEED_FILTER_TAU_MS_DEFAULT;
    config->wheel_diameter_mm = (float)DRIVE_WHEEL_DIAMETER_MM;
    config->counts_per_rev_x100[0] = DRIVE_WHEEL1_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[1] = DRIVE_WHEEL2_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[2] = DRIVE_WHEEL3_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[3] = DRIVE_WHEEL4_COUNTS_PER_REV_X100;
}

void motion_defaults_load_action_config(motion_action_config_t *config)
{
    if(config == 0)
    {
        return;
    }

    motion_action_default_config(config);
    config->wheel_diameter_mm = (float)DRIVE_WHEEL_DIAMETER_MM;
    config->counts_per_rev_x100[0] = DRIVE_WHEEL1_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[1] = DRIVE_WHEEL2_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[2] = DRIVE_WHEEL3_COUNTS_PER_REV_X100;
    config->counts_per_rev_x100[3] = DRIVE_WHEEL4_COUNTS_PER_REV_X100;
    config->move_max_speed_mm_s = MOTION_ACTION_MOVE_MAX_SPEED_MM_S;
    config->move_accel_mm_s2 = MOTION_ACTION_MOVE_ACCEL_MM_S2;
    config->move_kp_mm_s_per_mm = MOTION_ACTION_MOVE_KP_MM_S_PER_MM;
    config->move_approach_speed_mm_s = MOTION_ACTION_MOVE_APPROACH_SPEED_MM_S;
    config->rotate_max_speed_mm_s = MOTION_ACTION_ROTATE_MAX_SPEED_MM_S;
    config->rotate_accel_mm_s2 = MOTION_ACTION_ROTATE_ACCEL_MM_S2;
    config->rotate_kp_mm_s_per_deg = MOTION_ACTION_ROTATE_KP_MM_S_PER_DEG;
    config->rotate_approach_speed_mm_s = MOTION_ACTION_ROTATE_APPROACH_SPEED_MM_S;
    config->heading_hold_kp_mm_s_per_deg = MOTION_ACTION_HEADING_KP_MM_S_PER_DEG;
    config->heading_hold_max_rot_mm_s = MOTION_ACTION_HEADING_MAX_ROT_MM_S;
    config->move_done_error_mm = MOTION_ACTION_MOVE_DONE_ERROR_MM;
    config->rotate_done_error_deg = MOTION_ACTION_ROTATE_DONE_ERROR_DEG;
    config->wheel_stop_mm_s = MOTION_ACTION_WHEEL_STOP_MM_S;
    config->imu_stop_dps = MOTION_ACTION_IMU_STOP_DPS;
    config->completion_ms = MOTION_ACTION_COMPLETION_MS;
    config->timeout_ms = MOTION_ACTION_TIMEOUT_MS;
    config->obstruction_ms = MOTION_ACTION_OBSTRUCTION_MS;
    config->obstruction_command_mm_s = MOTION_ACTION_OBSTRUCTION_COMMAND_MM_S;
    config->odom_x_sign = MOTION_ACTION_ODOM_X_SIGN;
    config->odom_y_sign = MOTION_ACTION_ODOM_Y_SIGN;
}
