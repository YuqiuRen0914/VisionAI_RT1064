#ifndef ACTION_CONTROLLER_H
#define ACTION_CONTROLLER_H

#include <stdint.h>

#include "vision_protocol.h"

typedef struct
{
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;
} action_controller_frame_t;

typedef struct
{
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;
} action_controller_response_t;

typedef enum
{
    ACTION_CONTROLLER_MOTION_NONE = 0,
    ACTION_CONTROLLER_MOTION_START,
    ACTION_CONTROLLER_MOTION_STOP_ALL,
    ACTION_CONTROLLER_MOTION_RESET_ALL,
} action_controller_motion_event_t;

typedef struct
{
    action_controller_response_t response;
    action_controller_motion_event_t motion_event;
    action_controller_frame_t action;
} action_controller_result_t;

typedef struct
{
    uint8_t status;
    uint8_t has_last_action;
    uint8_t last_reply_valid;
    action_controller_frame_t last_action;
    action_controller_response_t last_reply;
    action_controller_frame_t active_action;
} action_controller_t;

void action_controller_init(action_controller_t *controller);
action_controller_result_t action_controller_handle_frame(action_controller_t *controller,
                                                          const action_controller_frame_t *frame);
action_controller_result_t action_controller_handle_protocol_error(action_controller_t *controller,
                                                                   uint8_t seq,
                                                                   uint8_t error,
                                                                   uint8_t context);
action_controller_result_t action_controller_complete_done(action_controller_t *controller,
                                                           uint32_t elapsed_ms);
action_controller_result_t action_controller_complete_error(action_controller_t *controller,
                                                            uint8_t error,
                                                            uint8_t context);
uint8_t action_controller_get_status(const action_controller_t *controller);
uint8_t action_controller_get_active_seq(const action_controller_t *controller);

#endif
