# Add condition session aggregation and derived conclusions

Status: ready-for-agent
Type: enhancement

## Parent

[PRD.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/PRD.md)

## What to build

Add the aggregation and session-judgment layer that turns run evidence into usable condition- and session-level outputs. This slice should:

- organize JSON into `session`, `conditions`, and `runs`
- generate Markdown with per-condition aggregate sections and per-run detail rows
- compute condition summaries using pass counts, quality counts, and median/min/max for manual fields
- derive canonical session `primary_status`
- keep session judgments session-local even when the same candidate label appears again later
- compute and expose the separate `quality_flag`

This slice is where the bench stops being “a pile of logs” and becomes a structured evidence engine for candidate comparison.

## Acceptance criteria

- [ ] JSON output is layered into `session`, `conditions`, and `runs`
- [ ] Markdown output contains both per-condition aggregate sections and per-run detail rows
- [ ] Per-condition aggregates use pass counts, quality counts, and median/min/max rather than mean-first reporting
- [ ] Session-level `primary_status` is derived according to the agreed rules for `accepted`, `scoped-pass`, `rejected`, `exploratory`, `partial`, and `aborted`
- [ ] `quality_flag` is produced as a separate field and triggered by the agreed concern rules
- [ ] Repeated sessions for the same candidate label are not auto-collapsed into a single cross-session verdict

## Blocked by

- [03-add-matrix-and-subset-session-planning.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/issues/03-add-matrix-and-subset-session-planning.md)
- [04-capture-richer-run-evidence-for-parameter-diagnosis.md](/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/action-effect-bench/issues/04-capture-richer-run-evidence-for-parameter-diagnosis.md)

