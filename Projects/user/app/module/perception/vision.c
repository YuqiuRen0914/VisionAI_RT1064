#include "vision.h"

#include "ai_config.h"
#include "motion.h"
#include "vision_protocol.h"

#define VISION_MOVE_DUTY              (20)
#define VISION_ROTATE_DUTY            (20)
#define VISION_MOVE_MS_PER_CM         (10U)
#define VISION_ROTATE_90_MS           (700U)
#define VISION_ROTATE_180_MS          (1400U)

typedef struct
{
    uint8_t active;
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint32_t elapsed_ms;
    uint32_t run_ms;
} vision_action_t;

static uint8_t vision_last_acked_seq;
static uint8_t vision_error_latched;
static vision_action_t vision_action;
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

static void vision_send_ack(const vision_protocol_frame_t *frame)
{
    vision_protocol_send_frame(frame->seq, VISION_PROTOCOL_CMD_ACK, frame->dir, frame->val);
}

static void vision_send_done(uint8_t seq, uint8_t dir, uint32_t elapsed_ms)
{
    uint32_t elapsed_10ms = elapsed_ms / 10U;

    if(elapsed_10ms > UINT8_MAX)
    {
        elapsed_10ms = UINT8_MAX;
    }

    vision_protocol_send_frame(seq, VISION_PROTOCOL_CMD_DONE, dir, (uint8_t)elapsed_10ms);
}

static void vision_send_error(uint8_t seq, uint8_t error, uint8_t context)
{
    vision_error_latched = 1U;
    vision_debug.last_error_seq = seq;
    vision_debug.last_error = error;
    vision_debug.last_error_context = context;

    if(error == VISION_PROTOCOL_ERROR_BAD_CMD)
    {
        vision_debug.bad_cmds++;
    }
    else if(error == VISION_PROTOCOL_ERROR_BUSY)
    {
        vision_debug.busy_errors++;
    }
    else if(error == VISION_PROTOCOL_ERROR_MOTOR_FAULT)
    {
        vision_debug.motion_errors++;
    }

    vision_protocol_send_frame(seq, VISION_PROTOCOL_CMD_ERROR, error, context);
}

static uint8_t vision_get_status(void)
{
    if(vision_action.active != 0U)
    {
        return VISION_PROTOCOL_STATUS_BUSY;
    }

    if(vision_error_latched != 0U)
    {
        return VISION_PROTOCOL_STATUS_ERROR;
    }

    return VISION_PROTOCOL_STATUS_IDLE;
}

static void vision_send_status(uint8_t seq)
{
    uint8_t current_seq = (vision_action.active != 0U) ? vision_action.seq : vision_last_acked_seq;

    vision_protocol_send_frame(seq, VISION_PROTOCOL_CMD_STATUS, vision_get_status(), current_seq);
}

static uint8_t vision_is_move_valid(const vision_protocol_frame_t *frame)
{
    if(frame->val == 0U)
    {
        return 0U;
    }

    return (frame->dir <= VISION_PROTOCOL_MOVE_RIGHT) ? 1U : 0U;
}

static uint8_t vision_is_rotate_valid(const vision_protocol_frame_t *frame)
{
    if(frame->dir > VISION_PROTOCOL_ROTATE_CW)
    {
        return 0U;
    }

    return ((frame->val == VISION_PROTOCOL_ROTATE_90_VAL) ||
            (frame->val == VISION_PROTOCOL_ROTATE_180_VAL)) ? 1U : 0U;
}

static ai_status_t vision_start_motion(const mecanum_velocity_t *velocity,
                                       uint32_t run_ms,
                                       const vision_protocol_frame_t *frame)
{
    ai_status_t status;

    if(motion_test_is_armed() == 0U)
    {
        (void)motion_test_arm();
    }

    status = motion_test_set_mecanum(velocity, run_ms);
    if(status != AI_OK)
    {
        return status;
    }

    vision_action.active = 1U;
    vision_action.seq = frame->seq;
    vision_action.cmd = frame->cmd;
    vision_action.dir = frame->dir;
    vision_action.elapsed_ms = 0;
    vision_action.run_ms = run_ms;
    vision_last_acked_seq = frame->seq;
    vision_error_latched = 0U;
    vision_send_ack(frame);

    return AI_OK;
}

static void vision_handle_move(const vision_protocol_frame_t *frame)
{
    mecanum_velocity_t velocity = {0, 0, 0};
    uint32_t run_ms;
    ai_status_t status;

    vision_debug.move_cmds++;

    if(vision_is_move_valid(frame) == 0U)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
        return;
    }

    if(vision_action.active != 0U)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BUSY, vision_action.seq);
        return;
    }

    if(frame->dir == VISION_PROTOCOL_MOVE_UP)
    {
        velocity.vx = VISION_MOVE_DUTY;
    }
    else if(frame->dir == VISION_PROTOCOL_MOVE_DOWN)
    {
        velocity.vx = (int16_t)-VISION_MOVE_DUTY;
    }
    else if(frame->dir == VISION_PROTOCOL_MOVE_LEFT)
    {
        velocity.vy = VISION_MOVE_DUTY;
    }
    else
    {
        velocity.vy = (int16_t)-VISION_MOVE_DUTY;
    }

    run_ms = (uint32_t)frame->val * VISION_MOVE_MS_PER_CM;
    status = vision_start_motion(&velocity, run_ms, frame);
    if(status != AI_OK)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_MOTOR_FAULT, (uint8_t)status);
    }
}

static void vision_handle_rotate(const vision_protocol_frame_t *frame)
{
    mecanum_velocity_t velocity = {0, 0, 0};
    uint32_t run_ms;
    ai_status_t status;

    vision_debug.rotate_cmds++;

    if(vision_is_rotate_valid(frame) == 0U)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
        return;
    }

    if(vision_action.active != 0U)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BUSY, vision_action.seq);
        return;
    }

    velocity.wz = (frame->dir == VISION_PROTOCOL_ROTATE_CCW) ? VISION_ROTATE_DUTY : (int16_t)-VISION_ROTATE_DUTY;
    run_ms = (frame->val == VISION_PROTOCOL_ROTATE_90_VAL) ? VISION_ROTATE_90_MS : VISION_ROTATE_180_MS;

    status = vision_start_motion(&velocity, run_ms, frame);
    if(status != AI_OK)
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_MOTOR_FAULT, (uint8_t)status);
    }
}

static void vision_handle_stop(const vision_protocol_frame_t *frame)
{
    vision_debug.stop_cmds++;

    if((frame->dir != 0U) || (frame->val != 0U))
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
        return;
    }

    (void)motion_test_stop();
    vision_action.active = 0U;
    vision_last_acked_seq = frame->seq;
    vision_error_latched = 0U;
    vision_send_ack(frame);
}

static void vision_handle_reset(const vision_protocol_frame_t *frame)
{
    vision_debug.reset_cmds++;

    if((frame->dir != 0U) || (frame->val != 0U))
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
        return;
    }

    (void)motion_test_stop();
    vision_action.active = 0U;
    vision_last_acked_seq = 0xFFU;
    vision_error_latched = 0U;
    vision_send_ack(frame);
}

static void vision_handle_query(const vision_protocol_frame_t *frame)
{
    vision_debug.query_cmds++;

    if((frame->dir != 0U) || (frame->val != 0U))
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
        return;
    }

    vision_send_status(frame->seq);
}

static void vision_handle_protocol_frame(const vision_protocol_frame_t *frame)
{
    if(frame->seq == vision_last_acked_seq)
    {
        vision_send_ack(frame);
        return;
    }

    if(frame->cmd == VISION_PROTOCOL_CMD_MOVE)
    {
        vision_handle_move(frame);
    }
    else if(frame->cmd == VISION_PROTOCOL_CMD_ROTATE)
    {
        vision_handle_rotate(frame);
    }
    else if(frame->cmd == VISION_PROTOCOL_CMD_STOP)
    {
        vision_handle_stop(frame);
    }
    else if(frame->cmd == VISION_PROTOCOL_CMD_QUERY)
    {
        vision_handle_query(frame);
    }
    else if(frame->cmd == VISION_PROTOCOL_CMD_RESET)
    {
        vision_handle_reset(frame);
    }
    else
    {
        vision_send_error(frame->seq, VISION_PROTOCOL_ERROR_BAD_CMD, frame->cmd);
    }
}

static void vision_handle_protocol_error(uint8_t seq, uint8_t error, uint8_t context)
{
    vision_send_error(seq, error, context);
}

static void vision_update_action(void)
{
    if(vision_action.active == 0U)
    {
        return;
    }

    if((vision_action.elapsed_ms + AI_VISION_PERIOD_MS) < vision_action.run_ms)
    {
        vision_action.elapsed_ms += AI_VISION_PERIOD_MS;
        return;
    }

    vision_action.elapsed_ms = vision_action.run_ms;
    vision_send_done(vision_action.seq, vision_action.dir, vision_action.elapsed_ms);
    vision_action.active = 0U;
}

ai_status_t vision_module_init(void)
{
    vision_last_acked_seq = 0xFFU;
    vision_error_latched = 0U;
    vision_action.active = 0U;
    vision_action.seq = 0;
    vision_action.cmd = 0;
    vision_action.dir = 0;
    vision_action.elapsed_ms = 0;
    vision_action.run_ms = 0;
    vision_clear_runtime_debug();

    return vision_protocol_init(vision_handle_protocol_frame, vision_handle_protocol_error);
}

void vision_module_tick(void)
{
    vision_protocol_tick();
    vision_update_action();
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
    debug->status = vision_get_status();
    debug->parser_state = protocol_debug.parser_state;
    debug->last_seq = protocol_debug.last_seq;
    debug->last_cmd = protocol_debug.last_cmd;
    debug->last_dir = protocol_debug.last_dir;
    debug->last_val = protocol_debug.last_val;
    debug->active_seq = vision_action.active ? vision_action.seq : 0xFFU;
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
