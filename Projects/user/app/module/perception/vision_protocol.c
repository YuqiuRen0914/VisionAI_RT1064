#include "vision_protocol.h"

#include <string.h>

#include "zf_common_interrupt.h"
#include "zf_driver_uart.h"

#define VISION_PROTOCOL_UART_INDEX             (UART_4)
#define VISION_PROTOCOL_UART_BAUDRATE          (115200U)
#define VISION_PROTOCOL_UART_TX_PIN            (UART4_TX_C16)
#define VISION_PROTOCOL_UART_RX_PIN            (UART4_RX_C17)

#define VISION_PROTOCOL_FRAME_BODY_LENGTH      (6U)
#define VISION_PROTOCOL_RX_BUFFER_SIZE         (64U)
#define VISION_PROTOCOL_DEBUG_RX_MASK          (VISION_PROTOCOL_DEBUG_LAST_RX_SIZE - 1U)

static volatile uint8_t vision_protocol_rx_buffer[VISION_PROTOCOL_RX_BUFFER_SIZE];
static volatile uint8_t vision_protocol_rx_read_index;
static volatile uint8_t vision_protocol_rx_write_index;
static volatile uint8_t vision_protocol_rx_overflow;

static uint8_t vision_protocol_parse_state;
static uint8_t vision_protocol_parse_body[VISION_PROTOCOL_FRAME_BODY_LENGTH];
static uint8_t vision_protocol_parse_index;
static vision_protocol_debug_t vision_protocol_debug;
static volatile uint8_t vision_protocol_debug_last_rx_index;
static vision_protocol_frame_handler_t vision_protocol_frame_handler;
static vision_protocol_error_handler_t vision_protocol_error_handler;

static void vision_protocol_reset_parser(void)
{
    vision_protocol_parse_state = VISION_PROTOCOL_PARSE_WAIT_SOF;
    vision_protocol_parse_index = 0;
}

static void vision_protocol_record_rx(uint8_t byte)
{
    vision_protocol_debug.rx_bytes++;
    vision_protocol_debug.last_rx[vision_protocol_debug_last_rx_index] = byte;
    vision_protocol_debug_last_rx_index = (uint8_t)((vision_protocol_debug_last_rx_index + 1U) & VISION_PROTOCOL_DEBUG_RX_MASK);

    if(vision_protocol_debug.last_rx_count < VISION_PROTOCOL_DEBUG_LAST_RX_SIZE)
    {
        vision_protocol_debug.last_rx_count++;
    }
}

uint8_t vision_protocol_checksum(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val)
{
    return (uint8_t)(seq ^ cmd ^ dir ^ val);
}

void vision_protocol_build_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val, uint8_t *frame)
{
    if(frame == 0)
    {
        return;
    }

    frame[0] = VISION_PROTOCOL_FRAME_SOF;
    frame[1] = seq;
    frame[2] = cmd;
    frame[3] = dir;
    frame[4] = val;
    frame[5] = vision_protocol_checksum(seq, cmd, dir, val);
    frame[6] = VISION_PROTOCOL_FRAME_EOF;
}

void vision_protocol_send_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val)
{
    uint8_t frame[VISION_PROTOCOL_FRAME_LENGTH];

    vision_protocol_build_frame(seq, cmd, dir, val, frame);
    uart_write_buffer(VISION_PROTOCOL_UART_INDEX, frame, VISION_PROTOCOL_FRAME_LENGTH);
}

static uint8_t vision_protocol_rx_used_count(void)
{
    if(vision_protocol_rx_write_index >= vision_protocol_rx_read_index)
    {
        return (uint8_t)(vision_protocol_rx_write_index - vision_protocol_rx_read_index);
    }

    return (uint8_t)(VISION_PROTOCOL_RX_BUFFER_SIZE - vision_protocol_rx_read_index + vision_protocol_rx_write_index);
}

static ai_status_t vision_protocol_enqueue_bytes(const uint8_t *data, uint32_t length)
{
    uint32_t primask;
    uint8_t free_count;
    uint32_t i;

    if((data == 0) || (length == 0U) || (length >= VISION_PROTOCOL_RX_BUFFER_SIZE))
    {
        return AI_ERR_INVALID_ARG;
    }

    primask = interrupt_global_disable();

    if(vision_protocol_rx_overflow != 0U)
    {
        interrupt_global_enable(primask);
        return AI_ERR_BUSY;
    }

    free_count = (uint8_t)((VISION_PROTOCOL_RX_BUFFER_SIZE - 1U) - vision_protocol_rx_used_count());
    if(length > free_count)
    {
        interrupt_global_enable(primask);
        return AI_ERR_BUSY;
    }

    for(i = 0; i < length; i++)
    {
        vision_protocol_record_rx(data[i]);
        vision_protocol_rx_buffer[vision_protocol_rx_write_index] = data[i];
        vision_protocol_rx_write_index = (uint8_t)((vision_protocol_rx_write_index + 1U) % VISION_PROTOCOL_RX_BUFFER_SIZE);
    }

    interrupt_global_enable(primask);
    return AI_OK;
}

ai_status_t vision_protocol_inject_frame(uint8_t seq, uint8_t cmd, uint8_t dir, uint8_t val, uint8_t *frame_out)
{
    uint8_t frame[VISION_PROTOCOL_FRAME_LENGTH];
    ai_status_t status;

    vision_protocol_build_frame(seq, cmd, dir, val, frame);
    status = vision_protocol_enqueue_bytes(frame, VISION_PROTOCOL_FRAME_LENGTH);
    if((status == AI_OK) && (frame_out != 0))
    {
        memcpy(frame_out, frame, VISION_PROTOCOL_FRAME_LENGTH);
    }

    return status;
}

static uint8_t vision_protocol_read_rx_byte(uint8_t *byte)
{
    if(vision_protocol_rx_read_index == vision_protocol_rx_write_index)
    {
        return 0U;
    }

    *byte = vision_protocol_rx_buffer[vision_protocol_rx_read_index];
    vision_protocol_rx_read_index = (uint8_t)((vision_protocol_rx_read_index + 1U) % VISION_PROTOCOL_RX_BUFFER_SIZE);

    return 1U;
}

static void vision_protocol_report_bad_frame(uint8_t seq, uint8_t context)
{
    vision_protocol_debug.bad_frames++;
    if(vision_protocol_error_handler != 0)
    {
        vision_protocol_error_handler(seq, VISION_PROTOCOL_ERROR_BAD_FRAME, context);
    }
}

static void vision_protocol_handle_rx_byte(uint8_t byte)
{
    vision_protocol_frame_t frame;
    uint8_t checksum;

    if(vision_protocol_parse_state == VISION_PROTOCOL_PARSE_WAIT_SOF)
    {
        if(byte == VISION_PROTOCOL_FRAME_SOF)
        {
            vision_protocol_parse_state = VISION_PROTOCOL_PARSE_READ_BODY;
            vision_protocol_parse_index = 0;
        }

        return;
    }

    vision_protocol_parse_body[vision_protocol_parse_index] = byte;
    vision_protocol_parse_index++;

    if(vision_protocol_parse_index < VISION_PROTOCOL_FRAME_BODY_LENGTH)
    {
        return;
    }

    frame.seq = vision_protocol_parse_body[0];
    frame.cmd = vision_protocol_parse_body[1];
    frame.dir = vision_protocol_parse_body[2];
    frame.val = vision_protocol_parse_body[3];
    checksum = vision_protocol_checksum(frame.seq, frame.cmd, frame.dir, frame.val);
    vision_protocol_reset_parser();

    if((vision_protocol_parse_body[5] != VISION_PROTOCOL_FRAME_EOF) || (vision_protocol_parse_body[4] != checksum))
    {
        vision_protocol_report_bad_frame(frame.seq, 0U);
        return;
    }

    vision_protocol_debug.frames_ok++;
    vision_protocol_debug.last_seq = frame.seq;
    vision_protocol_debug.last_cmd = frame.cmd;
    vision_protocol_debug.last_dir = frame.dir;
    vision_protocol_debug.last_val = frame.val;

    if(vision_protocol_frame_handler != 0)
    {
        vision_protocol_frame_handler(&frame);
    }
}

ai_status_t vision_protocol_init(vision_protocol_frame_handler_t frame_handler,
                                 vision_protocol_error_handler_t error_handler)
{
    vision_protocol_frame_handler = frame_handler;
    vision_protocol_error_handler = error_handler;
    vision_protocol_rx_read_index = 0;
    vision_protocol_rx_write_index = 0;
    vision_protocol_rx_overflow = 0;
    vision_protocol_debug_last_rx_index = 0;
    vision_protocol_reset_parser();
    vision_protocol_debug_clear();

    uart_init(VISION_PROTOCOL_UART_INDEX,
              VISION_PROTOCOL_UART_BAUDRATE,
              VISION_PROTOCOL_UART_TX_PIN,
              VISION_PROTOCOL_UART_RX_PIN);
    uart_rx_interrupt(VISION_PROTOCOL_UART_INDEX, 1U);

    return AI_OK;
}

void vision_protocol_tick(void)
{
    uint8_t byte;

    if(vision_protocol_rx_overflow != 0U)
    {
        vision_protocol_rx_overflow = 0U;
        vision_protocol_debug.rx_overflows++;
        vision_protocol_rx_read_index = vision_protocol_rx_write_index;
        vision_protocol_reset_parser();
        vision_protocol_report_bad_frame(0U, 0U);
    }

    while(vision_protocol_read_rx_byte(&byte) != 0U)
    {
        vision_protocol_handle_rx_byte(byte);
    }
}

void vision_protocol_uart_irq_handler(void)
{
    uint8 data;

    while(uart_query_byte(VISION_PROTOCOL_UART_INDEX, &data) != 0U)
    {
        uint8_t next_write_index = (uint8_t)((vision_protocol_rx_write_index + 1U) % VISION_PROTOCOL_RX_BUFFER_SIZE);

        if(next_write_index == vision_protocol_rx_read_index)
        {
            vision_protocol_rx_overflow = 1U;
            return;
        }

        vision_protocol_record_rx((uint8_t)data);
        vision_protocol_rx_buffer[vision_protocol_rx_write_index] = (uint8_t)data;
        vision_protocol_rx_write_index = next_write_index;
    }
}

void vision_protocol_debug_get(vision_protocol_debug_t *debug)
{
    uint8_t count;
    uint8_t start;
    uint8_t i;

    if(debug == 0)
    {
        return;
    }

    *debug = vision_protocol_debug;
    debug->parser_state = vision_protocol_parse_state;

    count = vision_protocol_debug.last_rx_count;
    if(count > VISION_PROTOCOL_DEBUG_LAST_RX_SIZE)
    {
        count = VISION_PROTOCOL_DEBUG_LAST_RX_SIZE;
    }

    start = (uint8_t)((vision_protocol_debug_last_rx_index + VISION_PROTOCOL_DEBUG_LAST_RX_SIZE - count) & VISION_PROTOCOL_DEBUG_RX_MASK);
    for(i = 0; i < count; i++)
    {
        debug->last_rx[i] = vision_protocol_debug.last_rx[(start + i) & VISION_PROTOCOL_DEBUG_RX_MASK];
    }
    debug->last_rx_count = count;
}

void vision_protocol_debug_clear(void)
{
    uint8_t i;

    vision_protocol_debug.rx_bytes = 0;
    vision_protocol_debug.rx_overflows = 0;
    vision_protocol_debug.frames_ok = 0;
    vision_protocol_debug.bad_frames = 0;
    vision_protocol_debug.parser_state = 0;
    vision_protocol_debug.last_seq = 0xFFU;
    vision_protocol_debug.last_cmd = 0;
    vision_protocol_debug.last_dir = 0;
    vision_protocol_debug.last_val = 0;
    vision_protocol_debug.last_rx_count = 0;
    vision_protocol_debug_last_rx_index = 0;

    for(i = 0; i < VISION_PROTOCOL_DEBUG_LAST_RX_SIZE; i++)
    {
        vision_protocol_debug.last_rx[i] = 0;
    }
}
