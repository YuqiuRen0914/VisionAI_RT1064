#include "comm_action_debug.h"

#include <string.h>

#include "comm_debug.h"
#include "motion.h"

static ai_status_t comm_action_debug_parse_move(motion_action_move_tuning_t *tuning)
{
    char *max_speed_text = strtok(0, " ");
    char *accel_text = strtok(0, " ");
    char *kp_text = strtok(0, " ");
    char *approach_text = strtok(0, " ");

    if((comm_debug_parse_f32(max_speed_text, &tuning->max_speed_mm_s) != AI_OK) ||
       (comm_debug_parse_f32(accel_text, &tuning->accel_mm_s2) != AI_OK) ||
       (comm_debug_parse_f32(kp_text, &tuning->kp_mm_s_per_mm) != AI_OK) ||
       (comm_debug_parse_f32(approach_text, &tuning->approach_speed_mm_s) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_action_debug_parse_rotate(motion_action_rotate_tuning_t *tuning)
{
    char *max_speed_text = strtok(0, " ");
    char *accel_text = strtok(0, " ");
    char *kp_text = strtok(0, " ");
    char *approach_text = strtok(0, " ");

    if((comm_debug_parse_f32(max_speed_text, &tuning->max_speed_mm_s) != AI_OK) ||
       (comm_debug_parse_f32(accel_text, &tuning->accel_mm_s2) != AI_OK) ||
       (comm_debug_parse_f32(kp_text, &tuning->kp_mm_s_per_deg) != AI_OK) ||
       (comm_debug_parse_f32(approach_text, &tuning->approach_speed_mm_s) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static ai_status_t comm_action_debug_parse_heading(motion_action_heading_tuning_t *tuning)
{
    char *kp_text = strtok(0, " ");
    char *max_rot_text = strtok(0, " ");

    if((comm_debug_parse_f32(kp_text, &tuning->kp_mm_s_per_deg) != AI_OK) ||
       (comm_debug_parse_f32(max_rot_text, &tuning->max_rot_mm_s) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

static void comm_action_debug_emit_snapshot(comm_action_debug_write_line_t write_line)
{
    motion_action_tuning_t tuning;

    motion_action_get_tuning(&tuning);
    comm_debug_send_line(write_line,
                         "DATA action_move max_speed_mm_s=%.1f accel_mm_s2=%.1f kp_mm_s_per_mm=%.3f approach_speed_mm_s=%.1f",
                         (double)tuning.move.max_speed_mm_s,
                         (double)tuning.move.accel_mm_s2,
                         (double)tuning.move.kp_mm_s_per_mm,
                         (double)tuning.move.approach_speed_mm_s);
    comm_debug_send_line(write_line,
                         "DATA action_rotate max_speed_mm_s=%.1f accel_mm_s2=%.1f kp_mm_s_per_deg=%.3f approach_speed_mm_s=%.1f",
                         (double)tuning.rotate.max_speed_mm_s,
                         (double)tuning.rotate.accel_mm_s2,
                         (double)tuning.rotate.kp_mm_s_per_deg,
                         (double)tuning.rotate.approach_speed_mm_s);
    comm_debug_send_line(write_line,
                         "DATA action_heading kp_mm_s_per_deg=%.3f max_rot_mm_s=%.1f",
                         (double)tuning.heading.kp_mm_s_per_deg,
                         (double)tuning.heading.max_rot_mm_s);
}

static void comm_action_debug_handle_move(comm_action_debug_write_line_t write_line)
{
    motion_action_move_tuning_t tuning;
    ai_status_t status;

    if(comm_action_debug_parse_move(&tuning) != AI_OK)
    {
        comm_debug_send_line(write_line, "ERR action move usage");
        return;
    }

    status = motion_action_set_move_tuning(&tuning);
    if(status == AI_OK)
    {
        comm_debug_send_line(write_line, "OK action move");
        comm_action_debug_emit_snapshot(write_line);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_debug_send_line(write_line, "ERR action busy");
    }
    else
    {
        comm_debug_send_line(write_line, "ERR action move bad_arg");
    }
}

static void comm_action_debug_handle_rotate(comm_action_debug_write_line_t write_line)
{
    motion_action_rotate_tuning_t tuning;
    ai_status_t status;

    if(comm_action_debug_parse_rotate(&tuning) != AI_OK)
    {
        comm_debug_send_line(write_line, "ERR action rotate usage");
        return;
    }

    status = motion_action_set_rotate_tuning(&tuning);
    if(status == AI_OK)
    {
        comm_debug_send_line(write_line, "OK action rotate");
        comm_action_debug_emit_snapshot(write_line);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_debug_send_line(write_line, "ERR action busy");
    }
    else
    {
        comm_debug_send_line(write_line, "ERR action rotate bad_arg");
    }
}

static void comm_action_debug_handle_heading(comm_action_debug_write_line_t write_line)
{
    motion_action_heading_tuning_t tuning;
    ai_status_t status;

    if(comm_action_debug_parse_heading(&tuning) != AI_OK)
    {
        comm_debug_send_line(write_line, "ERR action heading usage");
        return;
    }

    status = motion_action_set_heading_tuning(&tuning);
    if(status == AI_OK)
    {
        comm_debug_send_line(write_line, "OK action heading");
        comm_action_debug_emit_snapshot(write_line);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_debug_send_line(write_line, "ERR action busy");
    }
    else
    {
        comm_debug_send_line(write_line, "ERR action heading bad_arg");
    }
}

static void comm_action_debug_handle_defaults(comm_action_debug_write_line_t write_line)
{
    ai_status_t status = motion_action_restore_default_tuning();

    if(status == AI_OK)
    {
        comm_debug_send_line(write_line, "OK action defaults");
        comm_action_debug_emit_snapshot(write_line);
    }
    else if(status == AI_ERR_BUSY)
    {
        comm_debug_send_line(write_line, "ERR action busy");
    }
    else
    {
        comm_debug_send_line(write_line, "ERR action defaults");
    }
}

void comm_action_debug_handle(char *sub_command, comm_action_debug_write_line_t write_line)
{
    if(sub_command == 0)
    {
        comm_debug_send_line(write_line, "ERR action usage");
        return;
    }

    if(strcmp(sub_command, "show") == 0)
    {
        comm_action_debug_emit_snapshot(write_line);
    }
    else if(strcmp(sub_command, "move") == 0)
    {
        comm_action_debug_handle_move(write_line);
    }
    else if(strcmp(sub_command, "rotate") == 0)
    {
        comm_action_debug_handle_rotate(write_line);
    }
    else if(strcmp(sub_command, "heading") == 0)
    {
        comm_action_debug_handle_heading(write_line);
    }
    else if(strcmp(sub_command, "defaults") == 0)
    {
        comm_action_debug_handle_defaults(write_line);
    }
    else
    {
        comm_debug_send_line(write_line, "ERR action usage");
    }
}
