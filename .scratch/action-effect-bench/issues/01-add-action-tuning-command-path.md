# Add action tuning command path

Status: done
Type: enhancement

## Parent

[PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/PRD.md)

## What to build

Add the firmware-side action tuning path needed for first-pass **Temporary action tuning override**. This slice should introduce the **Action debug command module** in the **Debug text command layer**, expose explicit motion-layer wrapper APIs for action tuning, and make the following commands work end to end:

- `action move ...`
- `action rotate ...`
- `action heading ...`
- `action show`
- `action defaults`

The implementation must preserve the agreed first-pass boundaries:

- Only the move, rotate, and heading shape groups are mutable at runtime
- Completion and fault thresholds stay fixed
- Updates are idle-only and affect only later runs
- `action defaults` restores compiled defaults without reboot
- `action show` emits fixed parseable `DATA` lines
- Successful setters and defaults restore emit an `OK action ...` line plus a full tuning snapshot
- Bench-session action tuning state survives single-action completion, `QUERY`, `STOP`, and protocol `RESET`

This slice should stop at the command/runtime-tuning path itself. It does not need to include the host bench script yet.

## Acceptance criteria

- [x] The firmware exposes `action move`, `action rotate`, `action heading`, `action show`, and `action defaults` through a dedicated **Action debug command module**
- [x] Action tuning is routed through explicit motion-layer wrapper APIs rather than direct interface-layer access to runtime internals
- [x] Runtime tuning mutation is rejected while an action is active, but `action show` remains available in any motion mode
- [x] `action show` emits stable parseable `DATA` output for move, rotate, and heading groups
- [x] Successful setters and `action defaults` emit one `OK action ...` line followed by the full current tuning snapshot
- [x] Native tests cover command parsing, wrapper dispatch, idle-only rejection, defaults restore, and readback behavior

## Blocked by

None - can start immediately
