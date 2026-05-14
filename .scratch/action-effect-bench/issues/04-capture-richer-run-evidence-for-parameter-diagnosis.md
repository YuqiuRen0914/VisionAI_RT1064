# Capture richer run evidence for parameter diagnosis

Status: ready-for-agent
Type: enhancement

## Parent

[PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/PRD.md)

## What to build

Strengthen the run-level evidence so the bench is useful for AI-assisted parameter tuning, not just pass/fail recording. This slice should add:

- low-rate action status timeline capture during each run
- final board-side snapshot capture shaped for later analysis
- structured manual observation sign conventions and final-result semantics
- first-pass external threshold application for run-level `pass/fail`
- handling for missing manual observations so incomplete truth cannot be mistaken for accepted evidence

The goal is to preserve enough run-level detail that later tooling or an AI agent can infer why a candidate is bad, not just that it failed.

## Acceptance criteria

- [ ] Each run captures a low-rate action status timeline in addition to speed stream and final snapshots
- [ ] Manual observation fields follow the agreed command-frame sign convention and final-result semantics
- [ ] Run-level `pass/fail` is computed from the agreed first-pass external thresholds
- [ ] Missing manual observation is represented explicitly and blocks `accepted` or `scoped-pass`
- [ ] Run-level evidence in JSON/Markdown is rich enough to distinguish action-shape issues such as overshoot, slow convergence, heading tug, drift, and unstable settling

## Blocked by

- [02-ship-one-condition-action-bench-session-runner.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/issues/02-ship-one-condition-action-bench-session-runner.md)

