#ifndef VISION_PROTOCOL_H
#define VISION_PROTOCOL_H

#include "ai_error.h"

#define VISION_PROTOCOL_FRAME_SOF             (0xAAU)
#define VISION_PROTOCOL_FRAME_EOF             (0x55U)
#define VISION_PROTOCOL_FRAME_LENGTH          (7U)

#define VISION_PROTOCOL_CMD_MOVE              (0x01U)
#define VISION_PROTOCOL_CMD_ROTATE            (0x02U)
#define VISION_PROTOCOL_CMD_STOP              (0x03U)
#define VISION_PROTOCOL_CMD_QUERY             (0x04U)
#define VISION_PROTOCOL_CMD_RESET             (0x05U)
#define VISION_PROTOCOL_CMD_ACK               (0x81U)
#define VISION_PROTOCOL_CMD_DONE              (0x82U)
#define VISION_PROTOCOL_CMD_ERROR             (0x83U)
#define VISION_PROTOCOL_CMD_STATUS            (0x84U)

#define VISION_PROTOCOL_MOVE_UP               (0x00U)
#define VISION_PROTOCOL_MOVE_DOWN             (0x01U)
#define VISION_PROTOCOL_MOVE_LEFT             (0x02U)
#define VISION_PROTOCOL_MOVE_RIGHT            (0x03U)

#define VISION_PROTOCOL_ROTATE_CCW            (0x00U)
#define VISION_PROTOCOL_ROTATE_CW             (0x01U)
#define VISION_PROTOCOL_ROTATE_90_VAL         (0x5AU)
#define VISION_PROTOCOL_ROTATE_180_VAL        (0xB4U)

#define VISION_PROTOCOL_ERROR_OBSTRUCTED      (0x01U)
#define VISION_PROTOCOL_ERROR_MOTOR_FAULT     (0x02U)
#define VISION_PROTOCOL_ERROR_BAD_FRAME       (0x03U)
#define VISION_PROTOCOL_ERROR_BAD_CMD         (0x04U)
#define VISION_PROTOCOL_ERROR_TIMEOUT         (0x05U)
#define VISION_PROTOCOL_ERROR_BUSY            (0x06U)

#define VISION_PROTOCOL_STATUS_IDLE           (0x00U)
#define VISION_PROTOCOL_STATUS_BUSY           (0x01U)
#define VISION_PROTOCOL_STATUS_ERROR          (0x02U)

#define VISION_PROTOCOL_PARSE_WAIT_SOF        (0U)
#define VISION_PROTOCOL_PARSE_READ_BODY       (1U)

#define VISION_PROTOCOL_DEBUG_LAST_RX_SIZE    (16U)

typedef struct
{
    uint8_t seq;
    uint8_t cmd;
    uint8_t dir;
    uint8_t val;
} vision_protocol_frame_t;

typedef struct
{
    uint32_t rx_bytes;
    uint32_t rx_overflows;
    uint32_t frames_ok;
    uint32_t bad_frames;
    uint8_t parser_state;
    uint8_t last_seq;
    uint8_t last_cmd;
    uint8_t last_dir;
    uint8_t last_val;
    uint8_t last_rx_count;
    uint8_t last_rx[VISION_PROTOCOL_DEBUG_LAST_RX_SIZE];
} vision_protocol_debug_t;

typedef void (*vision_protocol_frame_handler_t)(const vision_protocol_frame_t *frame);
typedef void (*vision_protocol_error_handler_t)(uint8_t seq, uint8_t error, uint8_t context);

ai_status_t vision_protocol_init(vision_protocol_frame_handler_t frame_handler,
                                 vision_protocol_error_handler_t error_handler);
void vision_protocol_tick(void);
void vision_protocol_uart_irq_handler(void);

uint8_t vision_protocol_checksum(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val);
void vision_protocol_build_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val, uint8_t *frame);
void vision_protocol_send_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val);
ai_status_t vision_protocol_inject_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val, uint8_t *frame_out);

void vision_protocol_debug_get(vision_protocol_debug_t *debug);
void vision_protocol_debug_clear(void);

#endif
