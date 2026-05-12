# Implement ROTATE action feedback

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Implement closed-loop `ROTATE` actions using IMU X short-horizon heading as the primary angle feedback source and the wheel-speed PI loop as the inner loop.

`ROTATE` should use the measured body yaw sign for this robot: clockwise rotation is positive IMU X heading and counterclockwise rotation is negative. The controller should output `rot_mm_s` / `wz_wheel_mm_s` as a rotational wheel-speed contribution, not an angular velocity.

## Acceptance criteria

- [x] `ROTATE` accepts only 90 degree and 180 degree protocol values and rejects illegal direction/value combinations with `BAD_CMD`.
- [x] Each `ROTATE` action zeros or captures a short-horizon IMU X heading baseline at action start and does not accumulate global yaw across actions.
- [x] `ROTATE CW` targets positive short-horizon heading; `ROTATE CCW` targets negative short-horizon heading.
- [x] The rotation outer loop uses IMU X angle error to produce a bounded `rot_mm_s` / `wz_wheel_mm_s` contribution.
- [x] A conservative rotation profile limits acceleration, maximum rotational wheel-speed contribution, and final approach speed.
- [x] The mecanum solve converts `rot_mm_s` into four wheel-speed targets consumed by the wheel-speed PI loop.
- [x] Encoder-derived rotation evidence is used only for auxiliary fault checks, not as the primary rotation angle.
- [x] `DONE` is reported only after angle error is within the default rotation threshold, all four wheel speeds are near zero, IMU X angular rate is near zero, and the completion window remains stable for the configured duration.
- [x] `SENSOR_INVALID`, `OBSTRUCTED`, `TIMEOUT`, and `CONTROL_UNSTABLE` are attributed according to the agreed action fault attribution rules.
- [x] Debug telemetry or status output exposes enough rotation state to tune angle error, IMU X rate, target `rot_mm_s`, measured wheel speeds, and duty.
- [x] Native tests cover sign mapping, target angle selection, completion-window logic, and fault attribution where practical.
- [x] Keil build succeeds.

## Blocked by

- `.scratch/closed-loop-control/issues/03-add-speed-loop-bench-mode.md`
- `.scratch/closed-loop-control/issues/04-add-closed-loop-action-state-machine.md`

## Comments

- Implemented with TDD through `tests/native/test_motion_action.c`.
- Core action outer loop lives in `Projects/user/app/module/motion/motion_action.c`.
- Firmware wiring connects `motion_module_tick()` to encoder/IMU feedback, `motion_speed` wheel-speed PI targets, and `vision_module_tick()` DONE/ERROR completion polling.
- Debug evidence is available through `status` action lines plus existing `stream speed` target/measured/duty output.
- Verified native test: `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/common -IProjects/user/app/module/perception -IProjects/user/app/module/motion tests/native/test_motion_action.c Projects/user/app/module/motion/motion_action.c -o /tmp/test_motion_action && /tmp/test_motion_action`
- Verified integration build: `./tools/keil-vscode.sh build` -> `0 Error(s), 0 Warning(s)`.
