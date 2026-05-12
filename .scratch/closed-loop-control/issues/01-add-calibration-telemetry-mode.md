# Add calibration telemetry mode

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Add first-phase calibration and telemetry support to the debug text command layer so a human can calibrate encoder counts per wheel revolution and validate IMU X-axis yaw observation before closed-loop actions are implemented.

This should use local debug text commands, not the OPENART binary UART action protocol. Calibration results are printed only and are manually copied into source configuration.

## Acceptance criteria

- [x] A command resets the encoder calibration baseline for all wheels.
- [x] A command prints current per-wheel encoder totals or deltas for inspection.
- [x] A command accepts `wheel_id` and positive integer `turns`, then prints `counts`, `turns`, and `counts_per_rev_x100` for that wheel.
- [x] Invalid wheel IDs, missing arguments, zero turns, and non-integer turns return clear `ERR` responses.
- [x] A command zeros IMU X short-horizon yaw observation.
- [x] A stream mode prints IMU X angular rate and short-horizon integrated angle using fixed-point integer units such as `gx_x10` and `yawx_x10`.
- [x] Output values can be copied into source constants without manual divide-by-100 conversion.
- [x] Existing `stream enc5`, `stream enc100`, `stream imu_gyro`, and motor test commands keep working.

## Blocked by

None - can start immediately.

## Comments

- Implemented in TDD loop with native calibration tests and Keil build verification.
