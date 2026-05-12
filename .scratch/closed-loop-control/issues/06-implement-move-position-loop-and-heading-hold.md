# Implement MOVE position loop and heading hold

Status: ready-for-agent
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Implement closed-loop `MOVE` actions using single-action local odometry for `x/y` displacement, a trapezoid motion profile for commanded movement speed, and IMU X short-horizon heading hold for local yaw correction.

The UART `MOVE` distance remains centimeters at the protocol boundary, but the lower-controller action uses millimeters and millimeters per second internally.

## Acceptance criteria

- [ ] `MOVE` validates direction and nonzero distance, converts protocol centimeters to local millimeters, and rejects invalid values with `BAD_CMD`.
- [ ] Each `MOVE` action resets single-action local odometry at action start and discards that estimate after `DONE` or `ERROR`.
- [ ] Encoder odometry estimates local `x_mm` and `y_mm` from calibrated per-wheel travel.
- [ ] `MOVE UP` advances positive local `x`, `MOVE DOWN` advances negative local `x`, `MOVE LEFT` advances positive local `y`, and `MOVE RIGHT` advances negative local `y`, with configurable sign corrections for chassis wiring/roller direction.
- [ ] `LEFT` and `RIGHT` are native mecanum lateral moves, not composite rotate-then-move actions.
- [ ] The position controller uses the commanded axis displacement as feedback and does not use IMU heading as a distance source.
- [ ] A conservative trapezoid profile limits acceleration, maximum move speed, and final approach speed.
- [ ] Move heading hold captures the action-start short-horizon IMU X heading and outputs only a bounded `rot_mm_s` / `wz_wheel_mm_s` correction during the action.
- [ ] The mecanum solve combines `vx_mm_s`, `vy_mm_s`, and `rot_mm_s` into four wheel-speed targets consumed by the wheel-speed PI loop.
- [ ] `DONE` is reported only after position error is within the default move threshold, all four wheel speeds are near zero, and the completion window remains stable for the configured duration.
- [ ] `SENSOR_INVALID`, `OBSTRUCTED`, `TIMEOUT`, and `CONTROL_UNSTABLE` are attributed according to the agreed action fault attribution rules.
- [ ] Debug telemetry or status output exposes enough move state to tune target distance, measured axis displacement, heading error, target wheel speeds, measured wheel speeds, and duty.
- [ ] Native tests cover centimeter-to-millimeter conversion, direction sign mapping, odometry projection, completion-window logic, and fault attribution where practical.
- [ ] Keil build succeeds.

## Blocked by

- `.scratch/closed-loop-control/issues/03-add-speed-loop-bench-mode.md`
- `.scratch/closed-loop-control/issues/04-add-closed-loop-action-state-machine.md`
- `.scratch/closed-loop-control/issues/05-implement-rotate-action-feedback.md`

## Comments

