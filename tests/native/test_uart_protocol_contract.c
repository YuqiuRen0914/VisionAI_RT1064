#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vision_protocol.h"

static char *read_text_file(const char *path)
{
    FILE *fp;
    long size;
    char *buffer;
    size_t read_size;

    fp = fopen(path, "rb");
    if(fp == NULL)
    {
        printf("FAIL open %s\n", path);
        exit(1);
    }

    if(fseek(fp, 0, SEEK_END) != 0)
    {
        printf("FAIL seek %s\n", path);
        fclose(fp);
        exit(1);
    }

    size = ftell(fp);
    if(size < 0)
    {
        printf("FAIL size %s\n", path);
        fclose(fp);
        exit(1);
    }

    if(fseek(fp, 0, SEEK_SET) != 0)
    {
        printf("FAIL rewind %s\n", path);
        fclose(fp);
        exit(1);
    }

    buffer = (char *)malloc((size_t)size + 1U);
    if(buffer == NULL)
    {
        printf("FAIL alloc %s\n", path);
        fclose(fp);
        exit(1);
    }

    read_size = fread(buffer, 1U, (size_t)size, fp);
    if(read_size != (size_t)size)
    {
        printf("FAIL read %s\n", path);
        free(buffer);
        fclose(fp);
        exit(1);
    }

    buffer[size] = '\0';
    fclose(fp);
    return buffer;
}

static void expect_u8(const char *name, uint8_t actual, uint8_t expected)
{
    if(actual != expected)
    {
        printf("FAIL %s: expected 0x%02X got 0x%02X\n", name, expected, actual);
        exit(1);
    }
}

static void expect_contains(const char *name, const char *text, const char *needle)
{
    if(strstr(text, needle) == NULL)
    {
        printf("FAIL %s: missing `%s`\n", name, needle);
        exit(1);
    }
}

static void protocol_error_codes_remain_stable(void)
{
    expect_u8("CMD MOVE", VISION_PROTOCOL_CMD_MOVE, 0x01U);
    expect_u8("CMD ROTATE", VISION_PROTOCOL_CMD_ROTATE, 0x02U);
    expect_u8("CMD STOP", VISION_PROTOCOL_CMD_STOP, 0x03U);
    expect_u8("CMD QUERY", VISION_PROTOCOL_CMD_QUERY, 0x04U);
    expect_u8("CMD RESET", VISION_PROTOCOL_CMD_RESET, 0x05U);
    expect_u8("CMD ACK", VISION_PROTOCOL_CMD_ACK, 0x81U);
    expect_u8("CMD DONE", VISION_PROTOCOL_CMD_DONE, 0x82U);
    expect_u8("CMD ERROR", VISION_PROTOCOL_CMD_ERROR, 0x83U);
    expect_u8("CMD STATUS", VISION_PROTOCOL_CMD_STATUS, 0x84U);

    expect_u8("ERR OBSTRUCTED", VISION_PROTOCOL_ERROR_OBSTRUCTED, 0x01U);
    expect_u8("ERR MOTOR_FAULT", VISION_PROTOCOL_ERROR_MOTOR_FAULT, 0x02U);
    expect_u8("ERR BAD_FRAME", VISION_PROTOCOL_ERROR_BAD_FRAME, 0x03U);
    expect_u8("ERR BAD_CMD", VISION_PROTOCOL_ERROR_BAD_CMD, 0x04U);
    expect_u8("ERR TIMEOUT", VISION_PROTOCOL_ERROR_TIMEOUT, 0x05U);
    expect_u8("ERR BUSY", VISION_PROTOCOL_ERROR_BUSY, 0x06U);
    expect_u8("ERR SENSOR_INVALID", VISION_PROTOCOL_ERROR_SENSOR_INVALID, 0x07U);
    expect_u8("ERR CONTROL_UNSTABLE", VISION_PROTOCOL_ERROR_CONTROL_UNSTABLE, 0x08U);
}

static void protocol_doc_mentions_fault_code_extension(void)
{
    char *doc = read_text_file("Projects/user/app/module/perception/UART_PROTOCOL.md");

    expect_contains("doc sensor invalid", doc, "SENSOR_INVALID = 0x07");
    expect_contains("doc control unstable", doc, "CONTROL_UNSTABLE = 0x08");
    expect_contains("doc chassis type", doc, "麦克纳姆");
    expect_contains("doc up definition", doc, "车体物理前方");
    expect_contains("doc done feedback", doc, "仍只回耗时");
    expect_contains("doc passive status", doc, "不做异步 push");
    expect_contains("doc grid step", doc, "格距是上层规划参数");
    expect_contains("doc timeout boundary", doc, "TIMEOUT");
    expect_contains("doc obstructed boundary", doc, "OBSTRUCTED");
    expect_contains("doc motor fault boundary", doc, "MOTOR_FAULT");
    expect_contains("doc sensor invalid boundary", doc, "SENSOR_INVALID");
    expect_contains("doc control unstable boundary", doc, "CONTROL_UNSTABLE");
    expect_contains("doc version history extension", doc, "| v1.1 | 2026-05-12 |");

    free(doc);
}

int main(void)
{
    protocol_error_codes_remain_stable();
    protocol_doc_mentions_fault_code_extension();
    printf("PASS uart_protocol_contract\n");
    return 0;
}
