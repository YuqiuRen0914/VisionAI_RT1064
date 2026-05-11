#ifndef VISION_H
#define VISION_H

#include "ai_error.h"

#define VISION_DEBUG_LAST_RX_SIZE 16U

typedef struct
{
    uint32_t rx_bytes;
    uint32_t rx_overflows;
    uint32_t frames_ok;
    uint32_t bad_frames;
    uint32_t bad_cmds;
    uint32_t busy_errors;
    uint32_t motion_errors;
    uint32_t move_cmds;
    uint32_t rotate_cmds;
    uint32_t stop_cmds;
    uint32_t query_cmds;
    uint32_t reset_cmds;
    uint8_t status;
    uint8_t parser_state;
    uint8_t last_seq;
    uint8_t last_cmd;
    uint8_t last_dir;
    uint8_t last_val;
    uint8_t last_error_seq;
    uint8_t last_error;
    uint8_t last_error_context;
    uint8_t active_seq;
    uint8_t last_rx_count;
    uint8_t last_rx[VISION_DEBUG_LAST_RX_SIZE];
} vision_debug_t;

ai_status_t vision_module_init(void);
void vision_module_tick(void);
void vision_debug_get(vision_debug_t *debug);
void vision_debug_clear(void);

#endif
