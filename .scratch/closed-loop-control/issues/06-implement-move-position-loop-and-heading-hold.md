# Implement MOVE position loop and heading hold

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Implement closed-loop `MOVE` actions using single-action local odometry for `x/y` displacement, a trapezoid motion profile for commanded movement speed, and IMU X short-horizon heading hold for local yaw correction.

The UART `MOVE` distance remains centimeters at the protocol boundary, but the lower-controller action uses millimeters and millimeters per second internally.

## Acceptance criteria

- [x] `MOVE` validates direction and nonzero distance, converts protocol centimeters to local millimeters, and rejects invalid values with `BAD_CMD`.
- [x] Each `MOVE` action resets single-action local odometry at action start and discards that estimate after `DONE` or `ERROR`.
- [x] Encoder odometry estimates local `x_mm` and `y_mm` from calibrated per-wheel travel.
- [x] `MOVE UP` advances positive local `x`, `MOVE DOWN` advances negative local `x`, `MOVE LEFT` advances positive local `y`, and `MOVE RIGHT` advances negative local `y`, with configurable sign corrections for chassis wiring/roller direction.
- [x] `LEFT` and `RIGHT` are native mecanum lateral moves, not composite rotate-then-move actions.
- [x] The position controller uses the commanded axis displacement as feedback and does not use IMU heading as a distance source.
- [x] A conservative trapezoid profile limits acceleration, maximum move speed, and final approach speed.
- [x] Move heading hold captures the action-start short-horizon IMU X heading and outputs only a bounded `rot_mm_s` / `wz_wheel_mm_s` correction during the action.
- [x] The mecanum solve combines `vx_mm_s`, `vy_mm_s`, and `rot_mm_s` into four wheel-speed targets consumed by the wheel-speed PI loop.
- [x] `DONE` is reported only after position error is within the default move threshold, all four wheel speeds are near zero, and the completion window remains stable for the configured duration.
- [x] `SENSOR_INVALID`, `OBSTRUCTED`, `TIMEOUT`, and `CONTROL_UNSTABLE` are attributed according to the agreed action fault attribution rules.
- [x] Debug telemetry or status output exposes enough move state to tune target distance, measured axis displacement, heading error, target wheel speeds, measured wheel speeds, and duty.
- [x] Native tests cover centimeter-to-millimeter conversion, direction sign mapping, odometry projection, completion-window logic, and fault attribution where practical.
- [x] Keil build succeeds.

## Blocked by

- `.scratch/closed-loop-control/issues/03-add-speed-loop-bench-mode.md`
- `.scratch/closed-loop-control/issues/04-add-closed-loop-action-state-machine.md`
- `.scratch/closed-loop-control/issues/05-implement-rotate-action-feedback.md`

## Comments

- Implemented with TDD through `tests/native/test_motion_action.c`.
- `motion_action.c` owns single-action local odometry, MOVE axis feedback, heading hold, profile limits, completion windows, and fault attribution.
- `motion.c` wires the action output into the existing four-wheel `motion_speed` PI loop and stores DONE/ERROR results for `vision.c` to replay through the protocol state machine.
- Debug evidence is available through `status` action lines plus existing `stream speed` target/measured/duty output.
- Verified native test: `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/common -IProjects/user/app/module/perception -IProjects/user/app/module/motion tests/native/test_motion_action.c Projects/user/app/module/motion/motion_action.c -o /tmp/test_motion_action && /tmp/test_motion_action`
- Verified integration build: `./tools/keil-vscode.sh build` -> `0 Error(s), 0 Warning(s)`.
