#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ai_error.h"
#include "comm_speed_debug.h"
#include "motion.h"

typedef struct
{
    char last_line[160];
    ai_status_t next_status;
    uint32_t arm_calls;
    uint32_t stop_calls;
    uint32_t set_wheel_calls;
    uint8_t wheel_id;
    float wheel_target_mm_s;
} test_motion_stub_t;

static test_motion_stub_t stub;

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
    assert(strcmp(command, "speed") == 0);
    comm_speed_debug_handle(strtok(0, " "), capture_line);
}

ai_status_t motion_speed_bench_arm(void)
{
    stub.arm_calls++;
    return stub.next_status;
}

ai_status_t motion_speed_bench_stop(void)
{
    stub.stop_calls++;
    return stub.next_status;
}

uint8_t motion_speed_bench_is_armed(void)
{
    return 0U;
}

ai_status_t motion_speed_bench_set_wheel(uint8_t wheel_id, float target_mm_s)
{
    stub.set_wheel_calls++;
    stub.wheel_id = wheel_id;
    stub.wheel_target_mm_s = target_mm_s;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_all(float wheel1_mm_s,
                                       float wheel2_mm_s,
                                       float wheel3_mm_s,
                                       float wheel4_mm_s)
{
    (void)wheel1_mm_s;
    (void)wheel2_mm_s;
    (void)wheel3_mm_s;
    (void)wheel4_mm_s;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_pid(float kp, float ki, float kd)
{
    (void)kp;
    (void)ki;
    (void)kd;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_limits(float duty_limit_percent, float max_speed_mm_s)
{
    (void)duty_limit_percent;
    (void)max_speed_mm_s;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_static(float duty_percent, float threshold_mm_s)
{
    (void)duty_percent;
    (void)threshold_mm_s;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_feedforward(float duty_per_mm_s)
{
    (void)duty_per_mm_s;
    return stub.next_status;
}

ai_status_t motion_speed_bench_set_filter(float tau_ms)
{
    (void)tau_ms;
    return stub.next_status;
}

void motion_speed_bench_get_sample(motion_speed_sample_t *sample)
{
    (void)sample;
}

static void test_arm_command_reports_ok(void)
{
    char line[] = "speed arm";

    reset_stub();
    run_command(line);

    assert(stub.arm_calls == 1U);
    assert(strcmp(stub.last_line, "OK speed arm") == 0);
}

static void test_wheel_command_sets_target(void)
{
    char line[] = "speed wheel 2 123.5";

    reset_stub();
    run_command(line);

    assert(stub.set_wheel_calls == 1U);
    assert(stub.wheel_id == 2U);
    assert(stub.wheel_target_mm_s > 123.4f);
    assert(stub.wheel_target_mm_s < 123.6f);
    assert(strcmp(stub.last_line, "OK speed wheel=2 target_mm_s=123.5") == 0);
}

static void test_wheel_busy_reports_disarmed(void)
{
    char line[] = "speed wheel 1 50";

    reset_stub();
    stub.next_status = AI_ERR_BUSY;
    run_command(line);

    assert(strcmp(stub.last_line, "ERR speed disarmed, send speed arm") == 0);
}

static void test_bad_wheel_args_do_not_call_motion(void)
{
    char line[] = "speed wheel nope 50";

    reset_stub();
    run_command(line);

    assert(stub.set_wheel_calls == 0U);
    assert(strcmp(stub.last_line, "ERR speed wheel usage") == 0);
}

int main(void)
{
    test_arm_command_reports_ok();
    test_wheel_command_sets_target();
    test_wheel_busy_reports_disarmed();
    test_bad_wheel_args_do_not_call_motion();

    puts("PASS comm_speed_debug");
    return 0;
}
