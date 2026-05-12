# Implement ROTATE action feedback

Status: ready-for-agent
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Implement closed-loop `ROTATE` actions using IMU X short-horizon heading as the primary angle feedback source and the wheel-speed PI loop as the inner loop.

`ROTATE` should use the measured body yaw sign for this robot: clockwise rotation is positive IMU X heading and counterclockwise rotation is negative. The controller should output `rot_mm_s` / `wz_wheel_mm_s` as a rotational wheel-speed contribution, not an angular velocity.

## Acceptance criteria

- [ ] `ROTATE` accepts only 90 degree and 180 degree protocol values and rejects illegal direction/value combinations with `BAD_CMD`.
- [ ] Each `ROTATE` action zeros or captures a short-horizon IMU X heading baseline at action start and does not accumulate global yaw across actions.
- [ ] `ROTATE CW` targets positive short-horizon heading; `ROTATE CCW` targets negative short-horizon heading.
- [ ] The rotation outer loop uses IMU X angle error to produce a bounded `rot_mm_s` / `wz_wheel_mm_s` contribution.
- [ ] A conservative rotation profile limits acceleration, maximum rotational wheel-speed contribution, and final approach speed.
- [ ] The mecanum solve converts `rot_mm_s` into four wheel-speed targets consumed by the wheel-speed PI loop.
- [ ] Encoder-derived rotation evidence is used only for auxiliary fault checks, not as the primary rotation angle.
- [ ] `DONE` is reported only after angle error is within the default rotation threshold, all four wheel speeds are near zero, IMU X angular rate is near zero, and the completion window remains stable for the configured duration.
- [ ] `SENSOR_INVALID`, `OBSTRUCTED`, `TIMEOUT`, and `CONTROL_UNSTABLE` are attributed according to the agreed action fault attribution rules.
- [ ] Debug telemetry or status output exposes enough rotation state to tune angle error, IMU X rate, target `rot_mm_s`, measured wheel speeds, and duty.
- [ ] Native tests cover sign mapping, target angle selection, completion-window logic, and fault attribution where practical.
- [ ] Keil build succeeds.

## Blocked by

- `.scratch/closed-loop-control/issues/03-add-speed-loop-bench-mode.md`
- `.scratch/closed-loop-control/issues/04-add-closed-loop-action-state-machine.md`

## Comments

