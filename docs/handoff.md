# Handoff: calibration telemetry done, next likely hardware bench test

## What changed

Issue `01-add-calibration-telemetry-mode` is implemented and marked done:
[01-add-calibration-telemetry-mode.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/issues/01-add-calibration-telemetry-mode.md:1)

Avoid re-summarising the PRD and issue in detail; use these as source artifacts:

- [PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/PRD.md:1)
- [01-add-calibration-telemetry-mode.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/issues/01-add-calibration-telemetry-mode.md:1)

New/changed code:

- New calibration Module:
  [calibration_telemetry.h](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Projects/user/app/module/calibration/calibration_telemetry.h:1)
  [calibration_telemetry.c](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Projects/user/app/module/calibration/calibration_telemetry.c:1)
- Debug text command layer wiring:
  [comm.c](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Projects/user/app/module/interface/comm.c:269)
- IMU body yaw axis switched to X axis to match `CONTEXT.md`:
  [imu660ra.c](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Libraries/user/imu660ra/src/imu660ra.c:147)
  [imu660ra_config.h](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Libraries/user/imu660ra/inc/imu660ra_config.h:1)
- Placeholder fixed-point calibration constants added here:
  [drive_config.h](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Libraries/user/drive/inc/drive_config.h:63)
- Keil project updated to include the new Module:
  [AI_Vision_RT1064.uvprojx](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Projects/mdk/AI_Vision_RT1064.uvprojx:340)
- Native test added:
  [test_calibration_telemetry.c](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/tests/native/test_calibration_telemetry.c:1)

## Verified

- Native test passed:
  `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/common -IProjects/user/app/module tests/native/test_calibration_telemetry.c Projects/user/app/module/calibration/calibration_telemetry.c -o /tmp/calibration_telemetry_test`
  Then `/tmp/calibration_telemetry_test` -> `PASS calibration_telemetry`
- Keil build passed via `./tools/keil-vscode.sh build`
  Result: `0 Error(s), 0 Warning(s).`
- Hardware flash has **not** been confirmed in this handoff. Treat the board as not flashed until a fresh Keil download log shows `Verify OK` and serial `help` shows the new `cal` and `imu yawx` commands.

## Current debug commands

The new firmware exposes:

- `cal enc zero`
- `cal enc total`
- `cal enc delta`
- `cal enc wheel <1|2|3|4> <turns>`
- `imu yawx zero`
- `stream imu_yawx`

Existing commands like `stream enc5`, `stream enc100`, `stream imu_gyro`, `motor`, `move`, `arm`, `stop` were kept.

## Hardware flash and test status

If the next session is about real hardware testing:

- Current repo evidence confirms build success, but does **not** prove the physical board has already been flashed. Start by burning the new firmware again.
- Board testing requires the new firmware, because the new `cal ...` and `imu yawx ...` commands live in firmware.
- No burning is needed only for the native host-side test in `tests/native/`.
- Flash from VS Code with `Keil: Flash/Debug`, or from the terminal with `./tools/keil-vscode.sh flash` after the SEEKFREE CMSIS-DAP is attached to the Windows VM.
- Count flash as confirmed only when the download log includes `Verify OK` and the serial console at `115200 8N1` responds to `help` with `cal enc ...` and `imu yawx zero`.
- Write the hardware test log to:
  [hardware-test-results.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/hardware-test-results.md:1)
  Record the flash result, boot/help output, encoder counts for all wheels, IMU yawx sign observation, pass/fail notes, and any follow-up fixes.
- The calibration numbers should be copied manually into:
  [drive_config.h](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Libraries/user/drive/inc/drive_config.h:63)
  Specifically:
  `DRIVE_WHEEL1_COUNTS_PER_REV_X100`
  `DRIVE_WHEEL2_COUNTS_PER_REV_X100`
  `DRIVE_WHEEL3_COUNTS_PER_REV_X100`
  `DRIVE_WHEEL4_COUNTS_PER_REV_X100`
- There is still **no runtime persistence** and no auto-save to flash. The workflow is print values over serial, then manually edit the constants in source.

## Suggested hardware test flow

Use these references:

- [serial-self-check.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/docs/hardware/serial-self-check.md:1)
- [README-dev.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/docs/README-dev.md:1)

Practical sequence for the next agent:

1. Build and flash the board.
2. Open serial at `115200 8N1`.
3. Confirm boot text still appears, then run `help` and verify `cal enc ...` and `imu yawx zero` are listed.
4. Run `cal enc total` once to confirm encoder totals are readable.
5. Run `cal enc zero`.
6. Manually rotate one wheel a known positive integer number of turns.
7. Run `cal enc wheel <wheel_id> <turns>` and record `counts_per_rev_x100`.
8. Repeat for all 4 wheels.
9. Put those values into [drive_config.h](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/Libraries/user/drive/inc/drive_config.h:63), rebuild, and reflash if the constants are to be preserved in source.
10. For IMU check, run `imu yawx zero`, then `stream imu_yawx`, rotate the chassis around the body yaw axis, and verify `gx_x10` and `yawx_x10` move with the expected sign.

## Important caveats

- `Imu660raResetYaw()` now zeroes the short-horizon heading that is being integrated from gyro X, not gyro Z.
- `docs/hardware/serial-self-check.md` has not yet been updated with the new `cal` and `imu yawx` commands. If the next session is user-facing cleanup, updating that doc is a good next step.
- The issue is marked done, but actual **bench validation on hardware** has not been performed in this session.

## Worktree status

At handoff time, these changes were present and not committed:

- modified `.scratch/closed-loop-control/issues/01-add-calibration-telemetry-mode.md`
- modified `Libraries/user/drive/inc/drive_config.h`
- modified `Libraries/user/imu660ra/inc/imu660ra_config.h`
- modified `Libraries/user/imu660ra/src/imu660ra.c`
- modified `Projects/mdk/AI_Vision_RT1064.uvprojx`
- modified `Projects/user/app/module/interface/comm.c`
- untracked `Projects/user/app/module/calibration/`
- untracked `.scratch/closed-loop-control/hardware-test-results.md`
- untracked `tests/`

## Suggested skills for next session

- `diagnose` if hardware signs, encoder polarity, or IMU axis behavior look wrong on bench
- `tdd` if extending the calibration Module or adding more host-side tests
