# PRD: Automated Action-Effect Bench And Temporary Action Tuning Override

Status: ready-for-agent
Type: enhancement

## Problem Statement

The current repo has strong wheel-speed bench evidence and a working **Odometry action controller**, but there is no repeatable way to tune whole-chassis `MOVE` and `ROTATE` behavior as **Discrete actions**. A developer or AI agent can inject simulated vision commands today, but action-layer parameter iteration still lacks a dedicated runtime tuning surface, a structured bench workflow, and a durable evidence format that combines board-side telemetry with real-world measured outcome. As a result, tuning whole-action behavior is too manual, too slow, and too difficult to compare across candidate parameter sets.

## Solution

Build a first-version **Automated action-effect bench script** plus firmware-side **Temporary action tuning override** support for the action layer. The firmware will expose action tuning through the **Debug text command layer** with explicit `action` commands for move, rotate, heading, show, and defaults restore. The host bench will run over the existing **Mac-to-Windows serial bench workflow**, inject simulated `MOVE` and `ROTATE` **Vision decisions**, capture board-side evidence, prompt for structured real-world measurements after each run, and emit durable session artifacts plus an append-only long-term results table. The workflow will support both full-matrix and subset sessions, preserve canonical evidence rules, and derive consistent session-level conclusions such as `accepted`, `scoped-pass`, `rejected`, `exploratory`, `partial`, or `aborted`.

## User Stories

1. As a firmware developer, I want to change action-layer tuning at runtime, so that I can compare candidate parameters without rebuilding and reflashing on every trial.
2. As an AI agent, I want a narrow, explicit action tuning surface, so that I can adjust only intended action-shape parameters without accidentally weakening completion or fault boundaries.
3. As a human operator, I want each bench session to start from a known tuning baseline, so that stale prior-session state does not pollute new evidence.
4. As a bench operator, I want the script to reset UART action protocol state before each run, so that prior error or replay state does not leak into the next action sample.
5. As an AI agent, I want simulated `MOVE` and `ROTATE` injection over the existing serial path, so that I can exercise whole-chassis behavior without a live upper controller.
6. As a firmware developer, I want `action show` to return fixed parseable `DATA` lines, so that host automation can reliably snapshot the active action tuning.
7. As a firmware developer, I want successful action tuning setters to echo the full current tuning snapshot, so that every accepted runtime change is immediately auditable.
8. As a bench operator, I want action tuning state to persist across single actions, `QUERY`, `STOP`, and protocol `RESET`, so that one candidate session can cover multiple runs without reapplying the same tuning after every action.
9. As a firmware developer, I want action tuning updates to be idle-only, so that one evidence run never mixes multiple parameter sets.
10. As an AI agent, I want a canonical 12-condition first-pass matrix, so that candidate evidence is directly comparable across sessions.
11. As a bench operator, I want to run either the full matrix or a selected subset, so that exploratory work does not require all 36 standard runs every time.
12. As an AI agent, I want subset sessions to still execute in canonical order, so that partial evidence aligns with full-matrix evidence.
13. As a developer, I want standard sessions to repeat each selected condition three times, so that candidate acceptance is based on repeated evidence rather than one-off success.
14. As an AI agent, I want reduced-repeat exploratory sessions to be explicitly marked as nonstandard, so that they are not confused with standard acceptance evidence.
15. As a bench operator, I want the script to open `stream speed` only around an active run, so that telemetry remains aligned to one evidence window and prompts stay readable.
16. As an AI agent, I want a low-rate action status timeline during each run, so that I can diagnose overshoot, slow convergence, heading tug, and near-terminal instability instead of only seeing final snapshots.
17. As a bench operator, I want the script to stop after each run and ask for structured physical outcome data, so that real-world truth is captured while the run is still fresh.
18. As an AI agent, I want structured manual observation fields with fixed sign conventions, so that I can compare sessions programmatically without guessing what a value means.
19. As a bench operator, I want external `pass/fail` thresholds per first-pass bucket, so that session conclusions come from explicit rules instead of post-hoc intuition.
20. As an AI agent, I want per-condition aggregate summaries, so that I can quickly compare how a candidate behaves on each action family before diving into per-run detail.
21. As a bench operator, I want a Markdown session summary that is readable at a glance, so that I can review one whole candidate session without opening raw logs first.
22. As an AI agent, I want a structured JSON artifact with session, condition, and run layers, so that I can consume the same evidence without reparsing prose.
23. As a project maintainer, I want an append-only long-term results table, so that candidate history remains auditable and later sessions never rewrite older evidence.
24. As a project maintainer, I want each session to have an `AE` identifier, so that action-effect evidence does not collide with earlier speed-loop records.
25. As a developer, I want each session to correspond to exactly one candidate tuning set, so that one evidence session always maps to one hypothesis.
26. As a maintainer, I want candidate labels to be auto-generated from action tuning values, so that naming stays stable without extra operator work.
27. As a reviewer, I want long-term rows to include explicit coverage summaries, so that I can see whether a session covered the full matrix or only a subset.
28. As a reviewer, I want long-term rows to include a secondary quality flag, so that “pass but concerning” sessions are visible without inventing new primary statuses.
29. As a developer, I want standard full-matrix all-pass sessions to become `accepted`, so that a candidate can earn full first-pass acceptance.
30. As a developer, I want standard subset all-pass sessions to become `scoped-pass`, so that good partial evidence is visible without overstating coverage.
31. As a developer, I want standard-scope failures to reject a candidate immediately, so that a failing candidate is not kept alive merely because some conditions were untested.
32. As a maintainer, I want reduced-repeat sessions to remain `exploratory`, so that fast checks do not masquerade as acceptance evidence.
33. As a bench operator, I want incomplete but intact sessions to become `partial`, so that we can distinguish “stopped early” from “evidence chain broke”.
34. As a maintainer, I want transport or protocol-chain failures to become `aborted`, so that broken evidence is clearly separated from valid negative action results.
35. As a human operator, I want to record firmware, flash, surface, and power metadata before a session starts, so that later readers understand the real-world conditions behind the evidence.
36. As a future AI agent, I want run-level and condition-level evidence to stay separate but linked, so that I can use the right layer of detail when recommending the next parameter change.

## Implementation Decisions

- Add a firmware-side **Action debug command module** under the **Debug text command layer** to own `action move ...`, `action rotate ...`, `action heading ...`, `action show`, and `action defaults`.
- Keep the action tuning surface intentionally narrow in first pass: move shape (`max_speed`, `accel`, `kp`, `approach_speed`), rotate shape (`max_speed`, `accel`, `kp`, `approach_speed`), and heading correction (`kp`, `max_rot`).
- Do not expose completion thresholds, timeout thresholds, obstruction thresholds, or other fault-boundary controls through first-pass runtime tuning.
- Expose action tuning through explicit motion-layer wrapper APIs rather than a generic whole-config mutator, and do not couple the interface layer directly to runtime internals.
- Treat runtime action tuning as **Bench-session action tuning state** that persists across single-action completion, `QUERY`, **Emergency stop state**, and **Protocol reset**, and is cleared only by `action defaults` or reboot.
- Enforce idle-only mutation for action tuning, including `action defaults`, so accepted changes affect only later runs.
- Allow `action show` in any motion mode, but allow tuning mutation commands only when no action is active.
- Make `action show` emit fixed parseable `DATA` lines for move, rotate, and heading tuning groups.
- Make successful action tuning setters and `action defaults` return one `OK action ...` line followed by the full current tuning snapshot.
- Use `action defaults` rather than `action reset` to avoid name collision with **Protocol reset**.
- Keep the host bench on the existing **Mac-to-Windows serial bench workflow** and do not introduce direct BLE/macOS host ownership in the first pass.
- Build the host bench as an **Automated action-effect bench script** with a hardcoded first-pass matrix and explicit session rules rather than an open-ended configuration system.
- Start every bench session with `action defaults`, then `action show`, then apply the candidate’s action tuning overrides.
- Require manual session metadata before a run set starts: `firmware_label`, `flash_status`, `surface_label`, and `power_label`.
- Treat build and flash as external preconditions recorded in metadata rather than responsibilities of the bench script.
- Before every run, send `vision sim reset` to restore UART action protocol state to `IDLE` while preserving bench-session tuning state.
- Run each action sample inside a bounded telemetry window: enable `stream speed` immediately before the run and disable it immediately after run completion.
- Determine run completion by polling `vision` until protocol state leaves `BUSY` for `IDLE` or `ERROR`, then immediately capture final `vision` and `status` snapshots.
- Capture a low-rate action status timeline during each run by polling `status` on a coarse interval rather than trying to log control-rate snapshots.
- Use a default low-rate action status polling interval of `100 ms` for first pass.
- Define the first-pass canonical matrix as: `MOVE short` (`20 cm`) with `up/down/left/right`, `MOVE long` (`100 cm`) with `up/down/left/right`, `ROTATE 90` with `cw/ccw`, and `ROTATE 180` with `cw/ccw`.
- Preserve canonical condition order for full-matrix, subset, and round-robin execution.
- Default standard evidence to three repeats per selected condition and execute standard sessions round-robin by repetition index rather than three back-to-back runs of one condition.
- Allow subset sessions that target either whole first-pass buckets or individual concrete conditions.
- Allow reduced-repeat exploratory sessions, but mark them explicitly as nonstandard evidence and keep them under `exploratory`.
- Model one session as evidence for exactly one candidate tuning set; each `AE` session ID maps to one coherent move/rotate/heading trial.
- Auto-assign session IDs using an `AE01`/`AE02`/... namespace by default, with optional manual override for backfill or reruns.
- Auto-generate a stable candidate label from normalized move/rotate/heading parameter values only, using explicit group prefixes such as `m..._r..._h...`.
- Keep session conclusions session-local; repeated sessions for the same candidate label do not auto-collapse into one cross-session verdict in first pass.
- Derive the canonical `primary_status` from evidence rules rather than free-form writing.
- Reserve `accepted` for standard full-matrix sessions where every selected condition passes all three repeats.
- Reserve `scoped-pass` for standard subset sessions where the entire chosen scope passes all three repeats.
- Mark any standard-scope failure as `rejected`, even if the tested scope was only a subset of the full matrix.
- Reserve `partial` for incomplete sessions where the evidence chain stayed intact but the chosen scope was not fully run.
- Reserve `aborted` for broken evidence chains such as serial loss, unreadable protocol state, failed protocol reset, or board restart.
- Keep `good`/`acceptable`/`bad` as diagnostic manual grades rather than a second acceptance gate.
- Record manual observation using command-frame final-result semantics: final commanded-axis travel, final lateral displacement, final net rotation, signed final heading error, and final translation crosstalk magnitude.
- Use explicit first-pass external thresholds for `pass/fail` on `MOVE short`, `MOVE long`, `ROTATE 90`, and `ROTATE 180`.
- If a manual observation is intentionally skipped, record the run as missing manual truth and prevent the session from earning `accepted` or `scoped-pass`.
- Emit one raw serial log plus one JSON and one Markdown summary per session.
- Store all action bench artifacts under a dedicated action-effect scratch tree separate from speed-loop evidence.
- Structure the JSON artifact into top-level `session`, `conditions`, and `runs`.
- Structure the Markdown summary into session metadata, per-condition aggregate sections, then per-run detail rows.
- Use per-condition aggregates that emphasize `pass_count/total_runs`, quality counts, and `median/min/max` for manual fields rather than mean-first reporting on tiny repeat counts.
- Keep board-side final snapshot aggregation light at condition level, and preserve richer speed-stream evidence primarily at run level for later AI-assisted parameter adjustment.
- Maintain an append-only long-term results table with one row per completed, partial, or aborted session.
- Include in each long-term row: session ID, candidate label, primary status, coverage summary, quality flag, concise candidate parameter summary, and links or references to the session artifacts.
- Keep quality concern separate from primary status. Use a secondary quality flag such as `none` or `concern`.
- Trigger `quality_flag=concern` from any `bad` run grade, clearly unstable repeat-to-repeat spread within one condition, or explicit bench-visible anomaly notes such as slip, severe oscillation, or collision stop.
- Keep any explanatory rationale inline in the Markdown summary or long-term row note rather than creating a second free-standing session status field.

## Testing Decisions

- Good tests should validate externally visible behavior and stable contracts, not internal incidental implementation details.
- Firmware-side command-module tests should follow the existing native assert-based pattern already used for debug command modules: parse command text, call motion-layer wrappers, and assert on emitted response lines and wrapper call behavior.
- The new **Action debug command module** should receive native tests similar in style to the existing speed, motion, telemetry, and calibration command-module tests.
- Motion-layer action tuning wrappers should be tested at the behavior boundary: accepted updates in idle state, rejected updates during active action, defaults restore semantics, mode-tolerant access rules, and readback of current tuning groups.
- Existing native motion runtime tests provide prior art for adapter-driven runtime behavior and should be used as the model for any tuning-state tests that touch runtime sequencing.
- Protocol-layer behavior should continue to rely on the existing UART/action-controller contract style, rather than inventing a new ad-hoc testing style for action session state.
- Host-side bench logic should be split into pure helper modules wherever possible so matrix planning, canonical ordering, candidate label generation, session status derivation, quality-flag derivation, coverage summary generation, and artifact rendering can be tested without live serial hardware.
- Host-side summary generation should be covered by fixture-driven tests that feed representative run evidence into the aggregator and assert on JSON structure, Markdown summary content, and long-term row output.
- Host-side tests should focus on deterministic behavior: canonical order selection, repeat handling, state derivation, artifact naming, and summary rendering, not on mocking every byte of raw serial unless needed for parser coverage.
- There is limited prior art in this repo for Python-side bench tests, so the implementation should explicitly create small pure functions around planning, parsing, aggregation, and status derivation to keep host-side testing straightforward.

## Out of Scope

- Direct BLE/macOS ownership of the wireless transport.
- Automatic build and flash orchestration inside the action bench script.
- Runtime persistence of action tuning into flash or nonvolatile storage.
- Opening completion thresholds, timeout thresholds, or obstruction thresholds through the first-pass action tuning commands.
- Automatic cross-session candidate verdict synthesis beyond session-local `primary_status`.
- Automatic git snapshot provenance capture in the first pass.
- A separate AI-authored session conclusion field beyond the derived `primary_status` and inline notes.
- Full control-rate action-state capture.
- Peak-excursion manual metrics such as maximum lateral swing or maximum rotate translation excursion during the run.
- Per-direction parameter-class scheduling as a first-pass architecture decision.
- Any visual-servoing or camera-feedback control loop.

## Further Notes

- This PRD assumes the project vocabulary already stabilized in `CONTEXT.md` remains the authoritative language for implementation, logs, and summaries.
- The first-pass design deliberately separates session identity, candidate identity, session conditions, and session conclusion so future agents can reason about parameter quality without conflating “same tuning values” with “same experimental conditions.”
- The workflow is intentionally optimized for AI-assisted tuning comparison: dense run-level evidence is preserved where it matters, while higher-level summaries remain lightweight enough for humans to scan.
