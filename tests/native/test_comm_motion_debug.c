#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ai_error.h"
#include "comm_motion_debug.h"
#include "motion.h"

typedef struct
{
    char last_line[160];
    ai_status_t next_status;
    uint8_t armed;
    uint32_t arm_calls;
    uint32_t disarm_calls;
    uint32_t set_motor_calls;
    uint32_t set_all_calls;
    uint32_t set_mecanum_calls;
    uint8_t wheel_id;
    int16_t duty;
    uint32_t run_ms;
    mecanum_velocity_t velocity;
} test_motion_debug_stub_t;

static test_motion_debug_stub_t stub;

static void reset_stub(void)
{
    memset(&stub, 0, sizeof(stub));
    stub.next_status = AI_OK;
}

static void capture_line(const char *text)
{
    (void)snprintf(stub.last_line, sizeof(stub.last_line), "%s", text);
}

static void run_command(char *line)
{
    char *command = strtok(line, " ");
    assert(command != 0);

    if(strcmp(command, "arm") == 0)
    {
        comm_motion_debug_handle_arm(capture_line);
    }
    else if(strcmp(command, "disarm") == 0)
    {
        comm_motion_debug_handle_disarm(capture_line);
    }
    else if(strcmp(command, "motor") == 0)
    {
        comm_motion_debug_handle_motor(strtok(0, " "), capture_line);
    }
    else if(strcmp(command, "move") == 0)
    {
        comm_motion_debug_handle_move(strtok(0, " "), capture_line);
    }
    else
    {
        assert(0);
    }
}

ai_status_t motion_test_arm(void)
{
    stub.arm_calls++;
    stub.armed = 1U;
    return stub.next_status;
}

ai_status_t motion_test_disarm(void)
{
    stub.disarm_calls++;
    stub.armed = 0U;
    return stub.next_status;
}

uint8_t motion_test_is_armed(void)
{
    return stub.armed;
}

ai_status_t motion_test_set_motor(uint8_t wheel_id, int16_t duty_percent, uint32_t run_ms)
{
    stub.set_motor_calls++;
    stub.wheel_id = wheel_id;
    stub.duty = duty_percent;
    stub.run_ms = run_ms;
    return stub.next_status;
}

ai_status_t motion_test_set_all(int16_t wheel1_duty,
                                int16_t wheel2_duty,
                                int16_t wheel3_duty,
                                int16_t wheel4_duty,
                                uint32_t run_ms)
{
    assert(wheel1_duty == wheel2_duty);
    assert(wheel2_duty == wheel3_duty);
    assert(wheel3_duty == wheel4_duty);
    stub.set_all_calls++;
    stub.duty = wheel1_duty;
    stub.run_ms = run_ms;
    return stub.next_status;
}

ai_status_t motion_test_set_mecanum(const mecanum_velocity_t *velocity, uint32_t run_ms)
{
    stub.set_mecanum_calls++;
    stub.velocity = *velocity;
    stub.run_ms = run_ms;
    return stub.next_status;
}

static void test_arm_and_disarm_report_ok(void)
{
    char arm_line[] = "arm";
    char disarm_line[] = "disarm";

    reset_stub();
    run_command(arm_line);
    assert(stub.arm_calls == 1U);
    assert(strcmp(stub.last_line, "OK arm") == 0);

    run_command(disarm_line);
    assert(stub.disarm_calls == 1U);
    assert(strcmp(stub.last_line, "OK disarm") == 0);
}

static void test_motor_all_sets_all_wheels(void)
{
    char line[] = "motor all 25 120";

    reset_stub();
    run_command(line);

    assert(stub.set_all_calls == 1U);
    assert(stub.duty == 25);
    assert(stub.run_ms == 120U);
    assert(strcmp(stub.last_line, "OK motor all duty=25 ms=120") == 0);
}

static void test_motor_busy_reports_disarmed(void)
{
    char line[] = "motor 2 30 100";

    reset_stub();
    stub.next_status = AI_ERR_BUSY;
    run_command(line);

    assert(stub.set_motor_calls == 1U);
    assert(stub.wheel_id == 2U);
    assert(strcmp(stub.last_line, "ERR motor disarmed, send arm") == 0);
}

static void test_move_left_sets_mecanum_velocity(void)
{
    char line[] = "move left 20 250";

    reset_stub();
    run_command(line);

    assert(stub.set_mecanum_calls == 1U);
    assert(stub.velocity.vx == 0);
    assert(stub.velocity.vy == 20);
    assert(stub.velocity.wz == 0);
    assert(stub.run_ms == 250U);
    assert(strcmp(stub.last_line, "OK move left duty=20 ms=250") == 0);
}

static void test_move_failure_reports_disarmed_when_not_armed(void)
{
    char line[] = "move fwd 25 100";

    reset_stub();
    stub.next_status = AI_ERR_BUSY;
    stub.armed = 0U;
    run_command(line);

    assert(strcmp(stub.last_line, "ERR move disarmed, send arm") == 0);
}

static void test_move_bad_mode_does_not_call_motion(void)
{
    char line[] = "move diagonal 25 100";

    reset_stub();
    run_command(line);

    assert(stub.set_mecanum_calls == 0U);
    assert(strcmp(stub.last_line, "ERR move bad_mode=diagonal") == 0);
}

int main(void)
{
    test_arm_and_disarm_report_ok();
    test_motor_all_sets_all_wheels();
    test_motor_busy_reports_disarmed();
    test_move_left_sets_mecanum_velocity();
    test_move_failure_reports_disarmed_when_not_armed();
    test_move_bad_mode_does_not_call_motion();

    puts("PASS comm_motion_debug");
    return 0;
}
