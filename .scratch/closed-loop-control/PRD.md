# Closed Loop Control Calibration Phase

Status: ready-for-agent
Type: enhancement

## Problem Statement

The lower controller must execute OPENART PLUS discrete actions accurately despite upstream vision and communication delays. Before implementing closed-loop `MOVE` and `ROTATE`, the project needs trustworthy encoder and IMU measurements.

## Solution

Add a calibration and telemetry phase on the debug text command layer. This phase exposes per-wheel encoder counts, integer-turn counts-per-revolution calibration, IMU X-axis short-horizon heading observation, and fixed-point values that can be copied directly into source configuration.

## Scope

- Use the debug text command layer in `comm.c`, not the OPENART binary action protocol.
- Print calibration results only; do not save to flash or runtime persistent storage.
- Use fixed-point integer outputs and matching source constants, such as `COUNTS_PER_REV_X100`.
- Keep OPENART action exchange focused on `MOVE`, `ROTATE`, `STOP`, `QUERY`, and `RESET`.

## Out of Scope

- Full closed-loop `MOVE` implementation.
- Full closed-loop `ROTATE` implementation.
- Runtime parameter persistence.
- Long-term global pose estimation.

## References

- `CONTEXT.md`
- `docs/adr/0001-use-local-odometry-action-controller.md`
- `Projects/user/app/module/perception/UART_PROTOCOL.md`

## Comments

