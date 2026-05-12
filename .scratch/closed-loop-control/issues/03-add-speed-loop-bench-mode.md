# Add speed-loop bench mode

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Add the first closed-loop motor-control tracer bullet: a debug text bench mode that commands per-wheel speed targets, estimates measured wheel speed from accumulated encoder totals over elapsed control time, and drives each wheel through a floating-point PI loop.

This mode is for human bench tuning before `MOVE` or `ROTATE` actions use the speed loop. It must not be exposed through the OPENART binary UART action protocol, and it must not persist tuning values to flash.

## Acceptance criteria

- [x] The motion control path has a millisecond time source suitable for computing actual control `dt` in the motion tick.
- [x] Wheel-speed observation uses accumulated encoder total delta over elapsed control time, not only the latest encoder interrupt delta.
- [x] Wheel targets and measured speeds use `mm/s`, with UART/debug output naming that unit clearly.
- [x] Four independent floating-point wheel-speed PI controllers drive PWM duty, with derivative, feedforward, and static-friction compensation fields reserved.
- [x] Static-friction compensation and feedforward are disabled by default.
- [x] The first-version closed-loop defaults include a conservative duty limit around 30 percent and a conservative speed limit around 200 mm/s.
- [x] Explicit stop, zero target stop, `STOP`, `RESET`, and error transitions clear wheel integrators and command zero duty.
- [x] Debug commands can arm/stop speed bench mode and command one wheel or all four wheels by target `mm/s`.
- [x] A speed stream reports target speed, measured speed, duty, encoder delta, and control `dt` for all four wheels.
- [x] Runtime tuning overrides can adjust PI gains and limits for bench testing, but reboot restores compile-time defaults.
- [x] Native tests cover wheel-speed conversion, PI limiting/anti-windup behavior, zero-target handling, and command parsing where practical.
- [x] Keil build succeeds.

## Blocked by

None - can start immediately.

## Comments

- Implemented with TDD through `tests/native/test_motion_speed.c`.
- Verified native test: `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/common -IProjects/user/app/module/motion tests/native/test_motion_speed.c Projects/user/app/module/motion/motion_speed.c -o /tmp/test_motion_speed && /tmp/test_motion_speed`
- Verified integration build: `./tools/keil-vscode.sh build` -> `0 Error(s), 0 Warning(s)`.
