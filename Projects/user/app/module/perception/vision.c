#include "vision.h"

#include "action_controller.h"
#include "motion.h"
#include "vision_protocol.h"

static action_controller_t vision_action_controller;
static vision_debug_t vision_debug;

static void vision_clear_runtime_debug(void)
{
    uint8_t i;

    vision_debug.rx_bytes = 0;
    vision_debug.rx_overflows = 0;
    vision_debug.frames_ok = 0;
    vision_debug.bad_frames = 0;
    vision_debug.bad_cmds = 0;
    vision_debug.busy_errors = 0;
    vision_debug.motion_errors = 0;
    vision_debug.move_cmds = 0;
    vision_debug.rotate_cmds = 0;
    vision_debug.stop_cmds = 0;
    vision_debug.query_cmds = 0;
    vision_debug.reset_cmds = 0;
    vision_debug.status = 0;
    vision_debug.parser_state = 0;
    vision_debug.last_seq = 0xFFU;
    vision_debug.last_cmd = 0;
    vision_debug.last_dir = 0;
    vision_debug.last_val = 0;
    vision_debug.last_error_seq = 0xFFU;
    vision_debug.last_error = 0;
    vision_debug.last_error_context = 0;
    vision_debug.active_seq = 0xFFU;
    vision_debug.last_rx_count = 0;

    for(i = 0; i < VISION_DEBUG_LAST_RX_SIZE; i++)
    {
        vision_debug.last_rx[i] = 0;
    }
}

static void vision_count_command(uint8_t cmd)
{
    if(cmd == VISION_PROTOCOL_CMD_MOVE)
    {
        vision_debug.move_cmds++;
    }
    else if(cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        vision_debug.rotate_cmds++;
    }
    else if(cmd == VISION_PROTOCOL_CMD_STOP)
    {
        vision_debug.stop_cmds++;
    }
    else if(cmd == VISION_PROTOCOL_CMD_QUERY)
    {
        vision_debug.query_cmds++;
    }
    else if(cmd == VISION_PROTOCOL_CMD_RESET)
    {
        vision_debug.reset_cmds++;
    }
}

static void vision_record_response(const action_controller_response_t *response)
{
    if(response->cmd != VISION_PROTOCOL_CMD_ERROR)
    {
        return;
    }

    vision_debug.last_error_seq = response->seq;
    vision_debug.last_error = response->dir;
    vision_debug.last_error_context = response->val;

    if(response->dir == VISION_PROTOCOL_ERROR_BAD_CMD)
    {
        vision_debug.bad_cmds++;
    }
    else if(response->dir == VISION_PROTOCOL_ERROR_BUSY)
    {
        vision_debug.busy_errors++;
    }
    else if((response->dir == VISION_PROTOCOL_ERROR_MOTOR_FAULT) ||
            (response->dir == VISION_PROTOCOL_ERROR_OBSTRUCTED) ||
            (response->dir == VISION_PROTOCOL_ERROR_TIMEOUT) ||
            (response->dir == VISION_PROTOCOL_ERROR_SENSOR_INVALID) ||
            (response->dir == VISION_PROTOCOL_ERROR_CONTROL_UNSTABLE))
    {
        vision_debug.motion_errors++;
    }
}

static void vision_send_response(const action_controller_response_t *response)
{
    if(response->cmd == 0U)
    {
        return;
    }

    vision_record_response(response);
    vision_protocol_send_frame(response->seq, response->cmd, response->dir, response->val);
}

static void vision_apply_motion_event(const action_controller_result_t *result)
{
    action_controller_result_t error_result;
    ai_status_t status;
    uint8_t error;

    if(result->motion_event == ACTION_CONTROLLER_MOTION_START)
    {
        status = motion_action_begin(result->action.cmd, result->action.dir, result->action.val);
        if(status != AI_OK)
        {
            if(status == AI_ERR_NO_DATA)
            {
                error = VISION_PROTOCOL_ERROR_SENSOR_INVALID;
            }
            else if(status == AI_ERR_INVALID_ARG)
            {
                error = VISION_PROTOCOL_ERROR_BAD_CMD;
            }
            else
            {
                error = VISION_PROTOCOL_ERROR_BUSY;
            }

            error_result = action_controller_complete_error(&vision_action_controller,
                                                            error,
                                                            (uint8_t)status);
            vision_send_response(&error_result.response);
        }
    }
    else if(result->motion_event == ACTION_CONTROLLER_MOTION_STOP_ALL)
    {
        motion_action_stop_all();
    }
    else if(result->motion_event == ACTION_CONTROLLER_MOTION_RESET_ALL)
    {
        motion_action_reset();
    }
}

static void vision_dispatch_result(const action_controller_result_t *result)
{
    if(result->motion_event == ACTION_CONTROLLER_MOTION_START)
    {
        vision_apply_motion_event(result);
        if(action_controller_get_status(&vision_action_controller) == VISION_PROTOCOL_STATUS_BUSY)
        {
            vision_send_response(&result->response);
        }
        return;
    }

    vision_apply_motion_event(result);
    vision_send_response(&result->response);
}

static void vision_poll_motion_result(void)
{
    motion_action_result_t motion_result;
    action_controller_result_t action_result;

    if(motion_action_take_result(&motion_result) == 0U)
    {
        return;
    }

    if(motion_result.kind == MOTION_ACTION_RESULT_DONE)
    {
        action_result = action_controller_complete_done(&vision_action_controller,
                                                        motion_result.elapsed_ms);
    }
    else if(motion_result.kind == MOTION_ACTION_RESULT_ERROR)
    {
        action_result = action_controller_complete_error(&vision_action_controller,
                                                         motion_result.error,
                                                         motion_result.context);
    }
    else
    {
        return;
    }

    vision_send_response(&action_result.response);
}

static void vision_handle_protocol_frame(const vision_protocol_frame_t *frame)
{
    action_controller_frame_t action_frame;
    action_controller_result_t result;

    action_frame.seq = frame->seq;
    action_frame.cmd = frame->cmd;
    action_frame.dir = frame->dir;
    action_frame.val = frame->val;

    vision_count_command(frame->cmd);
    result = action_controller_handle_frame(&vision_action_controller, &action_frame);
    vision_dispatch_result(&result);
}

static void vision_handle_protocol_error(uint8_t seq, uint8_t error, uint8_t context)
{
    action_controller_result_t result;

    result = action_controller_handle_protocol_error(&vision_action_controller, seq, error, context);
    vision_dispatch_result(&result);
}

ai_status_t vision_module_init(void)
{
    action_controller_init(&vision_action_controller);
    vision_clear_runtime_debug();

    return vision_protocol_init(vision_handle_protocol_frame, vision_handle_protocol_error);
}

void vision_module_tick(void)
{
    vision_protocol_tick();
    vision_poll_motion_result();
}

void vision_debug_get(vision_debug_t *debug)
{
    vision_protocol_debug_t protocol_debug;
    uint8_t i;

    if(debug == 0)
    {
        return;
    }

    vision_protocol_debug_get(&protocol_debug);
    *debug = vision_debug;

    debug->rx_bytes = protocol_debug.rx_bytes;
    debug->rx_overflows = protocol_debug.rx_overflows;
    debug->frames_ok = protocol_debug.frames_ok;
    debug->bad_frames = protocol_debug.bad_frames;
    debug->status = action_controller_get_status(&vision_action_controller);
    debug->parser_state = protocol_debug.parser_state;
    debug->last_seq = protocol_debug.last_seq;
    debug->last_cmd = protocol_debug.last_cmd;
    debug->last_dir = protocol_debug.last_dir;
    debug->last_val = protocol_debug.last_val;
    debug->active_seq = action_controller_get_active_seq(&vision_action_controller);
    debug->last_rx_count = protocol_debug.last_rx_count;

    for(i = 0; i < VISION_DEBUG_LAST_RX_SIZE; i++)
    {
        debug->last_rx[i] = protocol_debug.last_rx[i];
    }
}

void vision_debug_clear(void)
{
    vision_clear_runtime_debug();
    vision_protocol_debug_clear();
}
