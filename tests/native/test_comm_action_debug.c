#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ai_error.h"
#include "comm_action_debug.h"
#include "motion.h"

typedef struct
{
    char lines[8][192];
    uint32_t line_count;
    motion_action_tuning_t tuning;
    uint32_t set_move_calls;
    uint32_t set_rotate_calls;
    uint32_t set_heading_calls;
    uint32_t defaults_calls;
    motion_action_move_tuning_t requested_move;
    motion_action_rotate_tuning_t requested_rotate;
    motion_action_heading_tuning_t requested_heading;
    ai_status_t next_status;
} test_action_stub_t;

static test_action_stub_t stub;

static void reset_stub(void)
{
    memset(&stub, 0, sizeof(stub));
    stub.next_status = AI_OK;
    stub.tuning.move.max_speed_mm_s = 180.0f;
    stub.tuning.move.accel_mm_s2 = 600.0f;
    stub.tuning.move.kp_mm_s_per_mm = 3.0f;
    stub.tuning.move.approach_speed_mm_s = 40.0f;
    stub.tuning.rotate.max_speed_mm_s = 120.0f;
    stub.tuning.rotate.accel_mm_s2 = 400.0f;
    stub.tuning.rotate.kp_mm_s_per_deg = 4.0f;
    stub.tuning.rotate.approach_speed_mm_s = 30.0f;
    stub.tuning.heading.kp_mm_s_per_deg = 3.0f;
    stub.tuning.heading.max_rot_mm_s = 80.0f;
}

static void capture_line(const char *text)
{
    assert(stub.line_count < 8U);
    (void)snprintf(stub.lines[stub.line_count], sizeof(stub.lines[0]), "%s", text);
    stub.line_count++;
}

static void run_command(char *line)
{
    char *command = strtok(line, " ");

    assert(command != 0);
    assert(strcmp(command, "action") == 0);
    comm_action_debug_handle(strtok(0, " "), capture_line);
}

void motion_action_get_tuning(motion_action_tuning_t *tuning)
{
    *tuning = stub.tuning;
}

ai_status_t motion_action_set_move_tuning(const motion_action_move_tuning_t *tuning)
{
    stub.set_move_calls++;
    stub.requested_move = *tuning;
    if(stub.next_status == AI_OK)
    {
        stub.tuning.move = *tuning;
    }
    return stub.next_status;
}

ai_status_t motion_action_set_rotate_tuning(const motion_action_rotate_tuning_t *tuning)
{
    stub.set_rotate_calls++;
    stub.requested_rotate = *tuning;
    if(stub.next_status == AI_OK)
    {
        stub.tuning.rotate = *tuning;
    }
    return stub.next_status;
}

ai_status_t motion_action_set_heading_tuning(const motion_action_heading_tuning_t *tuning)
{
    stub.set_heading_calls++;
    stub.requested_heading = *tuning;
    if(stub.next_status == AI_OK)
    {
        stub.tuning.heading = *tuning;
    }
    return stub.next_status;
}

ai_status_t motion_action_restore_default_tuning(void)
{
    stub.defaults_calls++;
    return stub.next_status;
}

static void show_emits_parseable_snapshot(void)
{
    char line[] = "action show";

    reset_stub();
    run_command(line);

    assert(stub.line_count == 3U);
    assert(strcmp(stub.lines[0],
                  "DATA action_move max_speed_mm_s=180.0 accel_mm_s2=600.0 kp_mm_s_per_mm=3.000 approach_speed_mm_s=40.0") == 0);
    assert(strcmp(stub.lines[1],
                  "DATA action_rotate max_speed_mm_s=120.0 accel_mm_s2=400.0 kp_mm_s_per_deg=4.000 approach_speed_mm_s=30.0") == 0);
    assert(strcmp(stub.lines[2],
                  "DATA action_heading kp_mm_s_per_deg=3.000 max_rot_mm_s=80.0") == 0);
}

static void move_setter_dispatches_and_echoes_snapshot(void)
{
    char line[] = "action move 210 700 2.5 55";

    reset_stub();
    run_command(line);

    assert(stub.set_move_calls == 1U);
    assert(stub.requested_move.max_speed_mm_s > 209.9f);
    assert(stub.requested_move.max_speed_mm_s < 210.1f);
    assert(stub.requested_move.accel_mm_s2 > 699.9f);
    assert(stub.requested_move.accel_mm_s2 < 700.1f);
    assert(stub.requested_move.kp_mm_s_per_mm > 2.49f);
    assert(stub.requested_move.kp_mm_s_per_mm < 2.51f);
    assert(stub.requested_move.approach_speed_mm_s > 54.9f);
    assert(stub.requested_move.approach_speed_mm_s < 55.1f);
    assert(stub.line_count == 4U);
    assert(strcmp(stub.lines[0], "OK action move") == 0);
    assert(strcmp(stub.lines[1],
                  "DATA action_move max_speed_mm_s=210.0 accel_mm_s2=700.0 kp_mm_s_per_mm=2.500 approach_speed_mm_s=55.0") == 0);
}

static void rotate_setter_dispatches_and_echoes_snapshot(void)
{
    char line[] = "action rotate 160 500 3.25 35";

    reset_stub();
    run_command(line);

    assert(stub.set_rotate_calls == 1U);
    assert(stub.requested_rotate.max_speed_mm_s > 159.9f);
    assert(stub.requested_rotate.max_speed_mm_s < 160.1f);
    assert(stub.requested_rotate.accel_mm_s2 > 499.9f);
    assert(stub.requested_rotate.accel_mm_s2 < 500.1f);
    assert(stub.requested_rotate.kp_mm_s_per_deg > 3.24f);
    assert(stub.requested_rotate.kp_mm_s_per_deg < 3.26f);
    assert(stub.requested_rotate.approach_speed_mm_s > 34.9f);
    assert(stub.requested_rotate.approach_speed_mm_s < 35.1f);
    assert(stub.line_count == 4U);
    assert(strcmp(stub.lines[0], "OK action rotate") == 0);
    assert(strcmp(stub.lines[2],
                  "DATA action_rotate max_speed_mm_s=160.0 accel_mm_s2=500.0 kp_mm_s_per_deg=3.250 approach_speed_mm_s=35.0") == 0);
}

static void heading_setter_dispatches_and_echoes_snapshot(void)
{
    char line[] = "action heading 2.75 65";

    reset_stub();
    run_command(line);

    assert(stub.set_heading_calls == 1U);
    assert(stub.requested_heading.kp_mm_s_per_deg > 2.74f);
    assert(stub.requested_heading.kp_mm_s_per_deg < 2.76f);
    assert(stub.requested_heading.max_rot_mm_s > 64.9f);
    assert(stub.requested_heading.max_rot_mm_s < 65.1f);
    assert(stub.line_count == 4U);
    assert(strcmp(stub.lines[0], "OK action heading") == 0);
    assert(strcmp(stub.lines[3],
                  "DATA action_heading kp_mm_s_per_deg=2.750 max_rot_mm_s=65.0") == 0);
}

static void defaults_echoes_snapshot_after_restore(void)
{
    char line[] = "action defaults";

    reset_stub();
    run_command(line);

    assert(stub.defaults_calls == 1U);
    assert(stub.line_count == 4U);
    assert(strcmp(stub.lines[0], "OK action defaults") == 0);
    assert(strcmp(stub.lines[1],
                  "DATA action_move max_speed_mm_s=180.0 accel_mm_s2=600.0 kp_mm_s_per_mm=3.000 approach_speed_mm_s=40.0") == 0);
}

static void busy_mutation_reports_busy_without_snapshot(void)
{
    char line[] = "action move 210 700 2.5 55";

    reset_stub();
    stub.next_status = AI_ERR_BUSY;
    run_command(line);

    assert(stub.set_move_calls == 1U);
    assert(stub.line_count == 1U);
    assert(strcmp(stub.lines[0], "ERR action busy") == 0);
}

static void bad_args_do_not_dispatch(void)
{
    char line[] = "action heading nope 60";

    reset_stub();
    run_command(line);

    assert(stub.set_heading_calls == 0U);
    assert(stub.line_count == 1U);
    assert(strcmp(stub.lines[0], "ERR action heading usage") == 0);
}

int main(void)
{
    show_emits_parseable_snapshot();
    move_setter_dispatches_and_echoes_snapshot();
    rotate_setter_dispatches_and_echoes_snapshot();
    heading_setter_dispatches_and_echoes_snapshot();
    defaults_echoes_snapshot_after_restore();
    busy_mutation_reports_busy_without_snapshot();
    bad_args_do_not_dispatch();

    puts("PASS comm_action_debug");
    return 0;
}
