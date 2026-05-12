#include "action_controller.h"

static action_controller_result_t action_controller_empty_result(void)
{
    action_controller_result_t result;

    result.response.seq = 0;
    result.response.cmd = 0;
    result.response.dir = 0;
    result.response.val = 0;
    result.motion_event = ACTION_CONTROLLER_MOTION_NONE;
    result.action.seq = 0;
    result.action.cmd = 0;
    result.action.dir = 0;
    result.action.val = 0;

    return result;
}

static uint8_t action_controller_same_frame(const action_controller_frame_t *a,
                                            const action_controller_frame_t *b)
{
    return ((a->seq == b->seq) &&
            (a->cmd == b->cmd) &&
            (a->dir == b->dir) &&
            (a->val == b->val)) ? 1U : 0U;
}

static action_controller_response_t action_controller_response(uint8_t seq,
                                                               uint8_t cmd,
                                                               uint8_t dir,
                                                               uint8_t val)
{
    action_controller_response_t response;

    response.seq = seq;
    response.cmd = cmd;
    response.dir = dir;
    response.val = val;

    return response;
}

static void action_controller_remember_reply(action_controller_t *controller,
                                             action_controller_response_t response)
{
    controller->last_reply = response;
    controller->last_reply_valid = 1U;
}

static action_controller_result_t action_controller_ack(const action_controller_frame_t *frame)
{
    action_controller_result_t result = action_controller_empty_result();

    result.response = action_controller_response(frame->seq,
                                                 VISION_PROTOCOL_CMD_ACK,
                                                 frame->dir,
                                                 frame->val);
    return result;
}

static action_controller_result_t action_controller_error(uint8_t seq,
                                                          uint8_t error,
                                                          uint8_t context)
{
    action_controller_result_t result = action_controller_empty_result();

    result.response = action_controller_response(seq,
                                                 VISION_PROTOCOL_CMD_ERROR,
                                                 error,
                                                 context);
    return result;
}

static uint8_t action_controller_move_valid(const action_controller_frame_t *frame)
{
    if(frame->val == 0U)
    {
        return 0U;
    }

    return (frame->dir <= VISION_PROTOCOL_MOVE_RIGHT) ? 1U : 0U;
}

static uint8_t action_controller_rotate_valid(const action_controller_frame_t *frame)
{
    if(frame->dir > VISION_PROTOCOL_ROTATE_CW)
    {
        return 0U;
    }

    return ((frame->val == VISION_PROTOCOL_ROTATE_90_VAL) ||
            (frame->val == VISION_PROTOCOL_ROTATE_180_VAL)) ? 1U : 0U;
}

static uint8_t action_controller_simple_valid(const action_controller_frame_t *frame)
{
    return ((frame->dir == 0U) && (frame->val == 0U)) ? 1U : 0U;
}

static action_controller_result_t action_controller_bad_command(action_controller_t *controller,
                                                                uint8_t seq,
                                                                uint8_t context)
{
    action_controller_result_t result = action_controller_error(seq,
                                                                VISION_PROTOCOL_ERROR_BAD_CMD,
                                                                context);
    controller->status = VISION_PROTOCOL_STATUS_ERROR;
    return result;
}

static action_controller_result_t action_controller_start_action(action_controller_t *controller,
                                                                 const action_controller_frame_t *frame)
{
    action_controller_result_t result = action_controller_ack(frame);

    controller->status = VISION_PROTOCOL_STATUS_BUSY;
    controller->has_last_action = 1U;
    controller->last_action = *frame;
    controller->active_action = *frame;
    action_controller_remember_reply(controller, result.response);
    result.motion_event = ACTION_CONTROLLER_MOTION_START;
    result.action = *frame;

    return result;
}

void action_controller_init(action_controller_t *controller)
{
    if(controller == 0)
    {
        return;
    }

    controller->status = VISION_PROTOCOL_STATUS_IDLE;
    controller->has_last_action = 0U;
    controller->last_reply_valid = 0U;
    controller->last_action.seq = 0;
    controller->last_action.cmd = 0;
    controller->last_action.dir = 0;
    controller->last_action.val = 0;
    controller->last_reply.seq = 0;
    controller->last_reply.cmd = 0;
    controller->last_reply.dir = 0;
    controller->last_reply.val = 0;
    controller->active_action.seq = 0;
    controller->active_action.cmd = 0;
    controller->active_action.dir = 0;
    controller->active_action.val = 0;
}

action_controller_result_t action_controller_handle_frame(action_controller_t *controller,
                                                          const action_controller_frame_t *frame)
{
    action_controller_result_t result;

    if((controller == 0) || (frame == 0))
    {
        return action_controller_empty_result();
    }

    if(controller->has_last_action != 0U && frame->seq == controller->last_action.seq)
    {
        if(action_controller_same_frame(frame, &controller->last_action) != 0U)
        {
            result = action_controller_empty_result();
            result.response = controller->last_reply_valid ? controller->last_reply :
                                                           action_controller_response(frame->seq,
                                                                                      VISION_PROTOCOL_CMD_ACK,
                                                                                      frame->dir,
                                                                                      frame->val);
            return result;
        }

        return action_controller_bad_command(controller, frame->seq, frame->cmd);
    }

    if(frame->cmd == VISION_PROTOCOL_CMD_QUERY)
    {
        if(action_controller_simple_valid(frame) == 0U)
        {
            return action_controller_bad_command(controller, frame->seq, frame->cmd);
        }

        result = action_controller_empty_result();
        result.response = action_controller_response(frame->seq,
                                                     VISION_PROTOCOL_CMD_STATUS,
                                                     controller->status,
                                                     action_controller_get_active_seq(controller));
        return result;
    }

    if(frame->cmd == VISION_PROTOCOL_CMD_RESET)
    {
        if(action_controller_simple_valid(frame) == 0U)
        {
            return action_controller_bad_command(controller, frame->seq, frame->cmd);
        }

        action_controller_init(controller);
        result = action_controller_ack(frame);
        result.motion_event = ACTION_CONTROLLER_MOTION_RESET_ALL;
        return result;
    }

    if(frame->cmd == VISION_PROTOCOL_CMD_STOP)
    {
        if(action_controller_simple_valid(frame) == 0U)
        {
            return action_controller_bad_command(controller, frame->seq, frame->cmd);
        }

        controller->status = VISION_PROTOCOL_STATUS_ERROR;
        controller->active_action.seq = 0;
        controller->active_action.cmd = 0;
        controller->active_action.dir = 0;
        controller->active_action.val = 0;
        result = action_controller_ack(frame);
        result.motion_event = ACTION_CONTROLLER_MOTION_STOP_ALL;
        return result;
    }

    if(controller->status == VISION_PROTOCOL_STATUS_ERROR)
    {
        return action_controller_error(frame->seq, VISION_PROTOCOL_ERROR_BUSY, controller->status);
    }

    if(frame->cmd == VISION_PROTOCOL_CMD_MOVE)
    {
        if(action_controller_move_valid(frame) == 0U)
        {
            return action_controller_bad_command(controller, frame->seq, frame->cmd);
        }
    }
    else if(frame->cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        if(action_controller_rotate_valid(frame) == 0U)
        {
            return action_controller_bad_command(controller, frame->seq, frame->cmd);
        }
    }
    else
    {
        return action_controller_bad_command(controller, frame->seq, frame->cmd);
    }

    if(controller->status == VISION_PROTOCOL_STATUS_BUSY)
    {
        return action_controller_error(frame->seq,
                                       VISION_PROTOCOL_ERROR_BUSY,
                                       controller->active_action.seq);
    }

    return action_controller_start_action(controller, frame);
}

action_controller_result_t action_controller_handle_protocol_error(action_controller_t *controller,
                                                                   uint8_t seq,
                                                                   uint8_t error,
                                                                   uint8_t context)
{
    action_controller_result_t result = action_controller_error(seq, error, context);

    if(controller != 0)
    {
        controller->status = VISION_PROTOCOL_STATUS_ERROR;
    }

    return result;
}

action_controller_result_t action_controller_complete_done(action_controller_t *controller,
                                                           uint32_t elapsed_ms)
{
    uint32_t elapsed_10ms;
    action_controller_result_t result = action_controller_empty_result();

    if((controller == 0) || (controller->status != VISION_PROTOCOL_STATUS_BUSY))
    {
        return result;
    }

    elapsed_10ms = elapsed_ms / 10U;
    if(elapsed_10ms > UINT8_MAX)
    {
        elapsed_10ms = UINT8_MAX;
    }

    result.response = action_controller_response(controller->active_action.seq,
                                                 VISION_PROTOCOL_CMD_DONE,
                                                 controller->active_action.dir,
                                                 (uint8_t)elapsed_10ms);
    controller->status = VISION_PROTOCOL_STATUS_IDLE;
    action_controller_remember_reply(controller, result.response);
    return result;
}

action_controller_result_t action_controller_complete_error(action_controller_t *controller,
                                                            uint8_t error,
                                                            uint8_t context)
{
    action_controller_result_t result = action_controller_empty_result();

    if((controller == 0) || (controller->status != VISION_PROTOCOL_STATUS_BUSY))
    {
        return result;
    }

    result.response = action_controller_response(controller->active_action.seq,
                                                 VISION_PROTOCOL_CMD_ERROR,
                                                 error,
                                                 context);
    controller->status = VISION_PROTOCOL_STATUS_ERROR;
    action_controller_remember_reply(controller, result.response);
    return result;
}

uint8_t action_controller_get_status(const action_controller_t *controller)
{
    if(controller == 0)
    {
        return VISION_PROTOCOL_STATUS_ERROR;
    }

    return controller->status;
}

uint8_t action_controller_get_active_seq(const action_controller_t *controller)
{
    if(controller == 0)
    {
        return 0xFFU;
    }

    if(controller->status == VISION_PROTOCOL_STATUS_BUSY)
    {
        return controller->active_action.seq;
    }

    if(controller->has_last_action != 0U)
    {
        return controller->last_action.seq;
    }

    return 0xFFU;
}
