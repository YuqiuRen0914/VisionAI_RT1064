# Ship a one-condition action bench session runner

Status: done
Type: enhancement

## Parent

[PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/PRD.md)

## What to build

Build the smallest end-to-end host bench loop for the **Automated action-effect bench script**. This slice should support one session that runs one selected condition and records one or more runs through the full bench flow:

- collect required session metadata
- initialize session tuning with `action defaults` and `action show`
- apply one candidate tuning set
- issue `vision sim reset` before each run
- enable `stream speed` only for the run window
- inject one simulated `MOVE` or `ROTATE`
- poll `vision` until the run leaves `BUSY`
- capture final `vision` and `status` snapshots
- prompt for **Structured action-effect observation**
- emit one raw serial log, one JSON artifact, and one Markdown summary under the dedicated action bench artifact root

This is the first tracer-bullet host workflow. It does not yet need the full canonical matrix or long-term aggregation.

## Acceptance criteria

- [x] The host bench can run at least one selected action condition end to end over the existing **Mac-to-Windows serial bench workflow**
- [x] The bench requires `firmware_label`, `flash_status`, `surface_label`, and `power_label` before starting
- [x] Each run performs `vision sim reset`, bounded `stream speed`, `vision` completion polling, and final `status`/`vision` capture
- [x] Each run prompts for structured manual outcome fields immediately after completion
- [x] The session emits raw serial output plus JSON and Markdown artifacts under `.scratch/action-effect-bench/`
- [x] The workflow treats build/flash as external preconditions rather than bench-script responsibilities

## Blocked by

- [01-add-action-tuning-command-path.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/issues/01-add-action-tuning-command-path.md)
