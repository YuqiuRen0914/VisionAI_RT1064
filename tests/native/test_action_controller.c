#include <stdio.h>
#include <stdlib.h>

#include "action_controller.h"
#include "vision_protocol.h"

static void expect_u8(const char *name, uint8_t actual, uint8_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected 0x%02X got 0x%02X\n", name, expected, actual);
        exit(1);
    }
}

static void expect_event(const char *name,
                         action_controller_motion_event_t actual,
                         action_controller_motion_event_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected %ld got %ld\n", name, (long)expected, (long)actual);
        exit(1);
    }
}

static action_controller_frame_t frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val)
{
    action_controller_frame_t result;

    result.seq = seq;
    result.cmd = cmd;
    result.dir = dir;
    result.val = val;
    return result;
}

static void duplicate_active_action_replays_ack(void)
{
    action_controller_t controller;
    action_controller_result_t result;
    action_controller_frame_t move = frame(7U, VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 40U);

    action_controller_init(&controller);

    result = action_controller_handle_frame(&controller, &move);
    expect_u8("first response", result.response.cmd, VISION_PROTOCOL_CMD_ACK);
    expect_event("first event", result.motion_event, ACTION_CONTROLLER_MOTION_START);

    result = action_controller_handle_frame(&controller, &move);
    expect_u8("duplicate response", result.response.cmd, VISION_PROTOCOL_CMD_ACK);
    expect_event("duplicate event", result.motion_event, ACTION_CONTROLLER_MOTION_NONE);
    expect_u8("status busy", action_controller_get_status(&controller), VISION_PROTOCOL_STATUS_BUSY);
}

static void completed_action_replays_done(void)
{
    action_controller_t controller;
    action_controller_result_t result;
    action_controller_frame_t rotate = frame(9U, VISION_PROTOCOL_CMD_ROTATE, VISION_PROTOCOL_ROTATE_CW, VISION_PROTOCOL_ROTATE_90_VAL);

    action_controller_init(&controller);
    (void)action_controller_handle_frame(&controller, &rotate);

    result = action_controller_complete_done(&controller, 1230U);
    expect_u8("done cmd", result.response.cmd, VISION_PROTOCOL_CMD_DONE);
    expect_u8("done val", result.response.val, 123U);
    expect_u8("status idle", action_controller_get_status(&controller), VISION_PROTOCOL_STATUS_IDLE);

    result = action_controller_handle_frame(&controller, &rotate);
    expect_u8("replayed cmd", result.response.cmd, VISION_PROTOCOL_CMD_DONE);
    expect_u8("replayed val", result.response.val, 123U);
    expect_event("replayed event", result.motion_event, ACTION_CONTROLLER_MOTION_NONE);
}

static void same_seq_different_payload_is_bad_command(void)
{
    action_controller_t controller;
    action_controller_result_t result;
    action_controller_frame_t move = frame(3U, VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U);
    action_controller_frame_t changed = frame(3U, VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_DOWN, 10U);

    action_controller_init(&controller);
    (void)action_controller_handle_frame(&controller, &move);

    result = action_controller_handle_frame(&controller, &changed);
    expect_u8("bad cmd response", result.response.cmd, VISION_PROTOCOL_CMD_ERROR);
    expect_u8("bad cmd code", result.response.dir, VISION_PROTOCOL_ERROR_BAD_CMD);
    expect_event("bad cmd event", result.motion_event, ACTION_CONTROLLER_MOTION_NONE);
}

static void busy_action_rejects_new_action_and_query_reports_busy(void)
{
    action_controller_t controller;
    action_controller_result_t result;
    action_controller_frame_t move = frame(10U, VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U);
    action_controller_frame_t rotate = frame(11U, VISION_PROTOCOL_CMD_ROTATE, VISION_PROTOCOL_ROTATE_CW, VISION_PROTOCOL_ROTATE_90_VAL);
    action_controller_frame_t query = frame(12U, VISION_PROTOCOL_CMD_QUERY, 0U, 0U);

    action_controller_init(&controller);
    (void)action_controller_handle_frame(&controller, &move);

    result = action_controller_handle_frame(&controller, &rotate);
    expect_u8("busy response", result.response.cmd, VISION_PROTOCOL_CMD_ERROR);
    expect_u8("busy code", result.response.dir, VISION_PROTOCOL_ERROR_BUSY);
    expect_event("busy event", result.motion_event, ACTION_CONTROLLER_MOTION_NONE);

    result = action_controller_handle_frame(&controller, &query);
    expect_u8("query status cmd", result.response.cmd, VISION_PROTOCOL_CMD_STATUS);
    expect_u8("query status busy", result.response.dir, VISION_PROTOCOL_STATUS_BUSY);
    expect_u8("query active seq", result.response.val, 10U);
}

static void stop_enters_error_until_reset(void)
{
    action_controller_t controller;
    action_controller_result_t result;
    action_controller_frame_t move = frame(1U, VISION_PROTOCOL_CMD_MOVE, VISION_PROTOCOL_MOVE_UP, 10U);
    action_controller_frame_t stop = frame(2U, VISION_PROTOCOL_CMD_STOP, 0U, 0U);
    action_controller_frame_t reset = frame(3U, VISION_PROTOCOL_CMD_RESET, 0U, 0U);

    action_controller_init(&controller);
    (void)action_controller_handle_frame(&controller, &move);

    result = action_controller_handle_frame(&controller, &stop);
    expect_u8("stop ack", result.response.cmd, VISION_PROTOCOL_CMD_ACK);
    expect_event("stop event", result.motion_event, ACTION_CONTROLLER_MOTION_STOP_ALL);
    expect_u8("status error", action_controller_get_status(&controller), VISION_PROTOCOL_STATUS_ERROR);

    result = action_controller_handle_frame(&controller, &reset);
    expect_u8("reset ack", result.response.cmd, VISION_PROTOCOL_CMD_ACK);
    expect_event("reset event", result.motion_event, ACTION_CONTROLLER_MOTION_RESET_ALL);
    expect_u8("status idle", action_controller_get_status(&controller), VISION_PROTOCOL_STATUS_IDLE);
}

int main(void)
{
    duplicate_active_action_replays_ack();
    completed_action_replays_done();
    same_seq_different_payload_is_bad_command();
    busy_action_rejects_new_action_and_query_reports_busy();
    stop_enters_error_until_reset();
    printf("PASS action_controller\n");
    return 0;
}
