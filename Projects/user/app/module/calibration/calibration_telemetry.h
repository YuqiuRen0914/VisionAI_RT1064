#ifndef CALIBRATION_TELEMETRY_H
#define CALIBRATION_TELEMETRY_H

#include <stdint.h>

#include "ai_error.h"

typedef struct
{
    int32_t wheel1;
    int32_t wheel2;
    int32_t wheel3;
    int32_t wheel4;
} calibration_wheel_counts_t;

typedef struct
{
    calibration_wheel_counts_t baseline;
} calibration_encoder_state_t;

typedef struct
{
    int32_t counts;
    uint32_t turns;
    int32_t counts_per_rev_x100;
} calibration_encoder_turn_result_t;

typedef struct
{
    int32_t yawx_zero_x10;
} calibration_yawx_state_t;

typedef struct
{
    int32_t gx_x10;
    int32_t yawx_x10;
} calibration_yawx_sample_t;

void calibration_encoder_reset_baseline(calibration_encoder_state_t *state,
                                        const calibration_wheel_counts_t *total);
ai_status_t calibration_encoder_delta(const calibration_encoder_state_t *state,
                                      const calibration_wheel_counts_t *total,
                                      calibration_wheel_counts_t *delta);
ai_status_t calibration_encoder_counts_per_rev(const calibration_encoder_state_t *state,
                                               const calibration_wheel_counts_t *total,
                                               uint8_t wheel_id,
                                               uint32_t turns,
                                               calibration_encoder_turn_result_t *result);
void calibration_yawx_zero(calibration_yawx_state_t *state, int32_t current_yawx_x10);
ai_status_t calibration_yawx_sample(const calibration_yawx_state_t *state,
                                    int32_t gx_x10,
                                    int32_t current_yawx_x10,
                                    calibration_yawx_sample_t *sample);

#endif
