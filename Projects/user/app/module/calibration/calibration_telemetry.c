#include "calibration/calibration_telemetry.h"

static ai_status_t calibration_get_wheel_counts(const calibration_wheel_counts_t *counts,
                                                uint8_t wheel_id,
                                                int32_t *value)
{
    if((counts == 0) || (value == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    if(wheel_id == 1U)
    {
        *value = counts->wheel1;
    }
    else if(wheel_id == 2U)
    {
        *value = counts->wheel2;
    }
    else if(wheel_id == 3U)
    {
        *value = counts->wheel3;
    }
    else if(wheel_id == 4U)
    {
        *value = counts->wheel4;
    }
    else
    {
        return AI_ERR_INVALID_ARG;
    }

    return AI_OK;
}

void calibration_encoder_reset_baseline(calibration_encoder_state_t *state,
                                        const calibration_wheel_counts_t *total)
{
    if((state == 0) || (total == 0))
    {
        return;
    }

    state->baseline = *total;
}

ai_status_t calibration_encoder_delta(const calibration_encoder_state_t *state,
                                      const calibration_wheel_counts_t *total,
                                      calibration_wheel_counts_t *delta)
{
    if((state == 0) || (total == 0) || (delta == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    delta->wheel1 = total->wheel1 - state->baseline.wheel1;
    delta->wheel2 = total->wheel2 - state->baseline.wheel2;
    delta->wheel3 = total->wheel3 - state->baseline.wheel3;
    delta->wheel4 = total->wheel4 - state->baseline.wheel4;

    return AI_OK;
}

ai_status_t calibration_encoder_counts_per_rev(const calibration_encoder_state_t *state,
                                               const calibration_wheel_counts_t *total,
                                               uint8_t wheel_id,
                                               uint32_t turns,
                                               calibration_encoder_turn_result_t *result)
{
    int32_t current;
    int32_t baseline;

    if((state == 0) || (total == 0) || (result == 0) || (turns == 0U))
    {
        return AI_ERR_INVALID_ARG;
    }

    if((calibration_get_wheel_counts(total, wheel_id, &current) != AI_OK) ||
       (calibration_get_wheel_counts(&state->baseline, wheel_id, &baseline) != AI_OK))
    {
        return AI_ERR_INVALID_ARG;
    }

    result->counts = current - baseline;
    result->turns = turns;
    result->counts_per_rev_x100 = (int32_t)(((int64_t)result->counts * 100) / (int64_t)turns);

    return AI_OK;
}

void calibration_yawx_zero(calibration_yawx_state_t *state, int32_t current_yawx_x10)
{
    if(state == 0)
    {
        return;
    }

    state->yawx_zero_x10 = current_yawx_x10;
}

ai_status_t calibration_yawx_sample(const calibration_yawx_state_t *state,
                                    int32_t gx_x10,
                                    int32_t current_yawx_x10,
                                    calibration_yawx_sample_t *sample)
{
    if((state == 0) || (sample == 0))
    {
        return AI_ERR_INVALID_ARG;
    }

    sample->gx_x10 = gx_x10;
    sample->yawx_x10 = current_yawx_x10 - state->yawx_zero_x10;

    return AI_OK;
}
