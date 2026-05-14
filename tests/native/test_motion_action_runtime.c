#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motion_action_runtime.h"
#include "vision_protocol.h"

typedef struct
{
    motion_action_runtime_observation_t observation;
    ai_status_t observation_status;
    ai_status_t heading_status;
    float heading_deg;
    uint32_t now_ms;
    uint32_t apply_calls;
    motion_speed_wheel_float_t last_duty;
} test_runtime_stub_t;

static test_runtime_stub_t stub;

static void fail_u8(const char *name, uint8_t actual, uint8_t expected)
{
    printf("FAIL %s: expected %u got %u\n", name, (unsigned int)expected, (unsigned int)actual);
    exit(1);
}

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
        fail_u8(name, actual, expected);
    }
}

static void expect_nonzero_u32(const char *name, uint32_t actual)
{
    if(actual == 0U)
    {
        printf("FAIL %s: expected nonzero\n", name);
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

static void expect_near_f32(const char *name, float actual, float expected, float tolerance)
{
    float diff = actual - expected;

    if(diff < 0.0f)
    {
        diff = -diff;
    }

    if(diff > tolerance)
    {
        printf("FAIL %s: expected %.3f got %.3f\n", name, (double)expected, (double)actual);
        exit(1);
    }
}

static void expect_zero_duty(const char *name, const motion_speed_wheel_float_t *duty)
{
    if((duty->wheel1 != 0.0f) ||
       (duty->wheel2 != 0.0f) ||
       (duty->wheel3 != 0.0f) ||
       (duty->wheel4 != 0.0f))
    {
        printf("FAIL %s: expected zero duty got %.3f %.3f %.3f %.3f\n",
               name,
               (double)duty->wheel1,
               (double)duty->wheel2,
               (double)duty->wheel3,
               (double)duty->wheel4);
        exit(1);
    }
}

static ai_status_t stub_read_observation(motion_action_runtime_observation_t *observation)
{
    if(stub.observation_status != AI_OK)
    {
        return stub.observation_status;
    }

    *observation = stub.observation;
    return AI_OK;
}

static ai_status_t stub_read_heading(float *heading_deg)
{
    if(stub.heading_status != AI_OK)
    {
        return stub.heading_status;
    }

    *heading_deg = stub.heading_deg;
    return AI_OK;
}

static uint32_t stub_now_ms(void)
{
    return stub.now_ms;
}

static void stub_apply_duty(const motion_speed_wheel_float_t *duty_percent)
{
    stub.last_duty = *duty_percent;
    stub.apply_calls++;
}

static motion_action_runtime_adapter_t stub_adapter(void)
{
    motion_action_runtime_adapter_t adapter;

    adapter.read_observation = stub_read_observation;
    adapter.read_heading_deg = stub_read_heading;
    adapter.now_ms = stub_now_ms;
    adapter.apply_duty = stub_apply_duty;
    return adapter;
}

static void reset_stub(void)
{
    memset(&stub, 0, sizeof(stub));
    stub.observation_status = AI_OK;
    stub.heading_status = AI_OK;
    stub.heading_deg = 0.0f;
    stub.observation.imu_heading_deg = 0.0f;
    stub.observation.imu_rate_dps = 0.0f;
}

static void init_runtime(void)
{
    motion_action_runtime_adapter_t adapter = stub_adapter();

    expect_status("runtime init", motion_action_runtime_init(&adapter), AI_OK);
}

static void invalid_adapter_is_rejected(void)
{
    motion_action_runtime_adapter_t adapter = stub_adapter();

    reset_stub();
    expect_status("null adapter", motion_action_runtime_init(0), AI_ERR_INVALID_ARG);

    adapter.read_observation = 0;
    expect_status("missing observation adapter", motion_action_runtime_init(&adapter), AI_ERR_INVALID_ARG);
}

static void begin_reports_sensor_invalid_when_heading_adapter_fails(void)
{
    reset_stub();
    init_runtime();

    stub.heading_status = AI_ERR_NO_DATA;
    expect_status("begin missing heading",
                  motion_action_runtime_begin(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U),
                  AI_ERR_NO_DATA);
}

static void observation_failure_reports_sensor_invalid_result(void)
{
    motion_action_result_t result;

    reset_stub();
    init_runtime();

    expect_status("begin move",
                  motion_action_runtime_begin(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U),
                  AI_OK);

    stub.observation_status = AI_ERR_NO_DATA;
    motion_action_runtime_tick();

    expect_u8("take error result", motion_action_runtime_take_result(&result), 1U);
    expect_u8("result kind", result.kind, MOTION_ACTION_RESULT_ERROR);
    expect_u8("sensor invalid", result.error, VISION_PROTOCOL_ERROR_SENSOR_INVALID);
    expect_nonzero_u32("zero duty applied", stub.apply_calls);
    expect_zero_duty("zero duty after sensor invalid", &stub.last_duty);
}

static void runtime_uses_adapter_time_observation_and_duty_output(void)
{
    motion_speed_sample_t speed_sample;
    motion_action_debug_t debug;

    reset_stub();
    init_runtime();

    expect_status("begin move",
                  motion_action_runtime_begin(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U),
                  AI_OK);
    stub.apply_calls = 0U;

    stub.now_ms = 100U;
    motion_action_runtime_tick();
    expect_u8("no first-sample duty", (uint8_t)stub.apply_calls, 0U);

    stub.now_ms = 120U;
    motion_action_runtime_tick();
    expect_u8("first active duty call", (uint8_t)stub.apply_calls, 1U);
    expect_zero_duty("first active duty uses prior zero target", &stub.last_duty);

    stub.now_ms = 140U;
    motion_action_runtime_tick();
    expect_u8("second active duty call", (uint8_t)stub.apply_calls, 2U);
    expect_positive("wheel1 duty", stub.last_duty.wheel1);
    expect_positive("wheel2 duty", stub.last_duty.wheel2);
    expect_positive("wheel3 duty", stub.last_duty.wheel3);
    expect_positive("wheel4 duty", stub.last_duty.wheel4);

    motion_action_runtime_get_speed_sample(&speed_sample);
    expect_u8("speed sample dt", (uint8_t)speed_sample.dt_ms, 20U);

    motion_action_runtime_get_debug(&debug);
    expect_u8("debug elapsed", (uint8_t)debug.elapsed_ms, 40U);
}

static void tuning_updates_are_read_back_when_idle(void)
{
    motion_action_tuning_t tuning;
    motion_action_move_tuning_t move = {210.0f, 700.0f, 2.5f, 55.0f};
    motion_action_rotate_tuning_t rotate = {160.0f, 500.0f, 3.25f, 35.0f};
    motion_action_heading_tuning_t heading = {2.75f, 65.0f};

    reset_stub();
    init_runtime();

    expect_status("set move tuning", motion_action_runtime_set_move_tuning(&move), AI_OK);
    expect_status("set rotate tuning", motion_action_runtime_set_rotate_tuning(&rotate), AI_OK);
    expect_status("set heading tuning", motion_action_runtime_set_heading_tuning(&heading), AI_OK);

    motion_action_runtime_get_tuning(&tuning);
    expect_near_f32("move max", tuning.move.max_speed_mm_s, 210.0f, 0.001f);
    expect_near_f32("move accel", tuning.move.accel_mm_s2, 700.0f, 0.001f);
    expect_near_f32("move kp", tuning.move.kp_mm_s_per_mm, 2.5f, 0.001f);
    expect_near_f32("move approach", tuning.move.approach_speed_mm_s, 55.0f, 0.001f);
    expect_near_f32("rotate max", tuning.rotate.max_speed_mm_s, 160.0f, 0.001f);
    expect_near_f32("rotate accel", tuning.rotate.accel_mm_s2, 500.0f, 0.001f);
    expect_near_f32("rotate kp", tuning.rotate.kp_mm_s_per_deg, 3.25f, 0.001f);
    expect_near_f32("rotate approach", tuning.rotate.approach_speed_mm_s, 35.0f, 0.001f);
    expect_near_f32("heading kp", tuning.heading.kp_mm_s_per_deg, 2.75f, 0.001f);
    expect_near_f32("heading max rot", tuning.heading.max_rot_mm_s, 65.0f, 0.001f);
}

static void tuning_updates_are_rejected_while_action_is_active(void)
{
    motion_action_tuning_t tuning;
    motion_action_move_tuning_t move = {210.0f, 700.0f, 2.5f, 55.0f};

    reset_stub();
    init_runtime();

    expect_status("begin active move",
                  motion_action_runtime_begin(VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U),
                  AI_OK);

    expect_status("reject active tuning", motion_action_runtime_set_move_tuning(&move), AI_ERR_BUSY);
    expect_status("reject active defaults", motion_action_runtime_restore_default_tuning(), AI_ERR_BUSY);

    motion_action_runtime_get_tuning(&tuning);
    expect_near_f32("move max unchanged", tuning.move.max_speed_mm_s, 180.0f, 0.001f);
}

static void defaults_restore_compiled_action_tuning(void)
{
    motion_action_tuning_t tuning;
    motion_action_move_tuning_t move = {210.0f, 700.0f, 2.5f, 55.0f};

    reset_stub();
    init_runtime();

    expect_status("set move tuning", motion_action_runtime_set_move_tuning(&move), AI_OK);
    expect_status("restore defaults", motion_action_runtime_restore_default_tuning(), AI_OK);

    motion_action_runtime_get_tuning(&tuning);
    expect_near_f32("default move max", tuning.move.max_speed_mm_s, 180.0f, 0.001f);
    expect_near_f32("default move accel", tuning.move.accel_mm_s2, 600.0f, 0.001f);
    expect_near_f32("default move kp", tuning.move.kp_mm_s_per_mm, 3.0f, 0.001f);
    expect_near_f32("default move approach", tuning.move.approach_speed_mm_s, 40.0f, 0.001f);
}

int main(void)
{
    invalid_adapter_is_rejected();
    begin_reports_sensor_invalid_when_heading_adapter_fails();
    observation_failure_reports_sensor_invalid_result();
    runtime_uses_adapter_time_observation_and_duty_output();
    tuning_updates_are_read_back_when_idle();
    tuning_updates_are_rejected_while_action_is_active();
    defaults_restore_compiled_action_tuning();

    printf("PASS motion_action_runtime\n");
    return 0;
}
