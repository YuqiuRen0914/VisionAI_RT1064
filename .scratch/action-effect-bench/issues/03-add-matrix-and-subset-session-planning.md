# Add matrix and subset session planning

Status: done
Type: enhancement

## Parent

[PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/PRD.md)

## What to build

Extend the host bench so sessions can plan and execute the agreed first-pass matrix and subset scopes. This slice should introduce:

- the 12-condition canonical matrix
- canonical condition order
- standard three-repeat round-robin execution
- full-matrix and subset session scope selection
- subset targeting by bucket or by individual concrete condition
- reduced-repeat exploratory sessions
- automatic `AE` session ID allocation with optional manual override
- automatic candidate label generation from move/rotate/heading tuning values only

This slice should make the session planner produce stable, repeatable run plans and carry enough metadata for later conclusion derivation.

## Acceptance criteria

- [x] The bench supports the full agreed 12-condition canonical matrix with canonical order
- [x] The bench supports subset sessions by bucket and by individual concrete condition while preserving canonical order
- [x] Standard sessions run three repeats in round-robin order across selected conditions
- [x] Reduced-repeat exploratory sessions are supported and marked as nonstandard evidence
- [x] Session IDs are auto-assigned in the `AE01`/`AE02`/... namespace, with optional override
- [x] Candidate labels are auto-generated from normalized move/rotate/heading tuning values with explicit `m`/`r`/`h` group prefixes

## Blocked by

- [02-ship-one-condition-action-bench-session-runner.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/issues/02-ship-one-condition-action-bench-session-runner.md)
