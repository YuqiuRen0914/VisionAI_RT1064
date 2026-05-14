# AI Vision RT1064

This context describes the firmware control language for the RT1064 lower controller and the OPENART PLUS vision module.

## Language

**Upper vision chain**:
The delayed decision path where the field overhead camera sends global information to the wireless screen, OPENART PLUS recognizes the screen, and OPENART PLUS sends a decision to the lower controller.
_Avoid_: realtime visual feedback, camera inner loop

**Lower controller**:
The RT1064 firmware that receives vision decisions and executes local motion using motors, encoders, and IMU feedback.
_Avoid_: slave board, MCU when discussing control ownership

**Vision decision**:
A discrete movement or rotation command sent by OPENART PLUS over `VISION_PROTOCOL`.
_Avoid_: realtime setpoint, continuous visual feedback

**Discrete action**:
One `MOVE` or `ROTATE` command from `UART_PROTOCOL.md` that the lower controller must finish before the upper controller sends the next motion command.
_Avoid_: continuous velocity command, streaming control command

**Action response replay**:
The UART retry behavior where a repeated action frame with the same `SEQ/CMD/DIR/VAL` never re-executes and instead replays `ACK` while active or the prior final `DONE`/`ERROR` after completion.
_Avoid_: re-running repeated actions, ACK-only completed retries

**Emergency stop state**:
The lower-controller state entered by UART `STOP`, where motors stop immediately and normal actions remain blocked until `RESET` clears the error state.
_Avoid_: normal action cancel, pause/resume

**Protocol reset**:
The UART `RESET` behavior that clears error state, current action state, and action response replay memory without rebooting the MCU or changing calibration constants.
_Avoid_: hardware reset, calibration erase

**Local closed loop**:
The lower-controller feedback loop that uses encoders and IMU data to make a vision decision execute steadily and accurately.
_Avoid_: camera closed loop

**Odometry action controller**:
The local controller that executes one discrete action using encoder-derived chassis displacement, IMU short-horizon heading, speed control, motion profiling, completion checks, and fault detection.
_Avoid_: fixed-time runner, open-loop action player

**Closed-loop action runtime**:
The lower-controller runtime Module that owns hardware sampling, the wheel-speed PI loop instance, action result handoff, duty application, and debug snapshots while an **Odometry action controller** executes one **Discrete action**.
_Avoid_: mixing action runtime ownership into debug modes, treating the pure action controller as the hardware owner

**Single-action local odometry**:
The encoder-derived displacement estimate reset at the start of one discrete action and discarded after `DONE` or `ERROR`.
_Avoid_: global pose, long-term localization

**Local motion unit**:
The lower-controller convention that local distance, displacement, and wheel-speed control use millimeters and millimeters per second after converting UART `MOVE` centimeters at the protocol boundary.
_Avoid_: centimeter control internals, raw encoder-count control units

**Wheel-speed observation**:
The raw measured wheel linear speed calculated in the motion tick from accumulated encoder total delta over elapsed control time and retained for telemetry and diagnosis.
_Avoid_: latest encoder interrupt sample, raw count-per-tick speed, filtered odometry distance

**Filtered wheel-speed estimate**:
The short-horizon control-facing wheel speed derived from **Wheel-speed observation** by light low-pass filtering before wheel-speed PI, start-boost exit, completion checks, and fault checks consume it.
_Avoid_: raw speed as PI input, filtering encoder position, hiding raw telemetry

**Wheel-speed PI loop**:
The first-version per-wheel speed controller that uses floating-point PI control, clears integral on explicit stop, reserves derivative and feedforward fields, and leaves static-friction compensation disabled by default.
_Avoid_: duty-only speed control, always-on static boost, derivative-first tuning

**Rotational wheel-speed contribution**:
The local `rot_mm_s` or `wz_wheel_mm_s` component that contributes opposite wheel linear-speed targets for chassis rotation.
_Avoid_: yaw rate, degrees per second, radians per second

**Encoder distance calibration**:
The measured conversion from encoder counts per wheel revolution and measured wheel diameter into wheel travel distance.
_Avoid_: ground-run calibration as the first step, guessed counts-per-centimeter

**Wheel travel calibration**:
The first-version calibration shape with one measured `counts_per_rev` per wheel and one shared measured wheel diameter for all wheels.
_Avoid_: one shared encoder count for every wheel

**Wheel layout**:
The physical mapping of drive wheel IDs: wheel 1 is front-right, wheel 2 is front-left, wheel 3 is rear-right, and wheel 4 is rear-left.
_Avoid_: assuming wheel ID order is front-left first

**Forward encoder direction**:
The encoder sign convention where positive encoder delta means that wheel is driving the chassis forward.
_Avoid_: raw encoder polarity

**Trapezoid motion profile**:
The accelerate-cruise-decelerate speed plan used to execute a discrete move or rotation without running full speed until the final target.
_Avoid_: fixed speed until target, duty ramp

**Completion window**:
The short stable period where the target error is small and measured wheel speed is near zero before a discrete action may report `DONE`.
_Avoid_: instant done, distance-only done, angle-only done

**Default completion thresholds**:
The first-version `DONE` thresholds: move error within 1.0 cm, rotation error within 2 degrees, wheel speeds near zero, rotation IMU angular rate near zero, and the condition stable for 100 ms.
_Avoid_: hardcoded final truth, tuned competition values

**Action fault state**:
The reason a discrete action reports `ERROR`, distinguishing timeout, obstruction, invalid sensor feedback, unstable control, motor fault, busy, bad command, and bad frame.
_Avoid_: generic motor fault for every failure

**Action fault attribution**:
The first-version error attribution order that prefers bad command or busy state, then invalid sensors, explicit motor fault evidence, sustained obstruction, hard timeout, and finally unstable control.
_Avoid_: severity-only ordering, reporting obstruction from invalid sensors, waiting for timeout before reporting clear obstruction

**Calibration telemetry mode**:
The first implementation phase that exposes encoder counts, wheel deltas, IMU X-axis angular velocity, short-horizon heading, and motor outputs before closed-loop actions are enabled.
_Avoid_: tuning blind, PID-first implementation

**Debug text command layer**:
The human-operated serial command interface in `comm.c` used for calibration, telemetry, and bench testing.
_Avoid_: OPENART action protocol for calibration commands

**Calibration debug command module**:
The debug text command Module that owns `cal enc ...` controls for **Encoder distance calibration**, including encoder baseline state, delta reporting, wheel-turn validation, and fixed-point calibration output.
_Avoid_: OPENART action protocol, hidden flash persistence, mixing encoder calibration state into action execution

**Motion debug command module**:
The debug text command Module that owns open-loop `arm`, `disarm`, `motor ...`, and `move ...` controls for the open-loop part of **Motion debug mode**.
_Avoid_: closed-loop action execution, speed-loop bench tuning, OPENART action protocol

**Telemetry debug command module**:
The debug text command Module that owns `stream ...` and `imu ...` controls for **Calibration telemetry mode**, including stream state, IMU stat windows, and periodic telemetry formatting.
_Avoid_: OPENART action protocol, mixing telemetry stream state into action execution

**Speed debug command module**:
The debug text command Module that exposes `speed ...` controls for **Speed-loop bench mode** and **Temporary tuning override** without owning the motion control loop.
_Avoid_: OPENART speed command, competition action protocol

**Vision debug command module**:
The debug text command Module that exposes human-operated `vision`, `vision clear`, and `vision sim ...` commands for inspecting UART action state and injecting simulated vision frames during bench testing.
_Avoid_: putting vision simulation parsing in the motion runtime, treating debug injection as an upper-controller protocol feature

**Speed-loop bench mode**:
The human-operated debug mode that commands wheel-speed targets and streams target speed, raw and filtered measured speed, duty, encoder delta, and control dt before closed-loop actions use the speed loop.
_Avoid_: OPENART speed command, direct competition action execution

**First-stage speed-loop acceptance**:
The bench acceptance standard that treats `±100 mm/s` and `±200 mm/s` as the main wheel-speed response checks, while `50 mm/s` only needs reliable start from rest and may remain visibly noisy until a later low-speed refinement pass.
_Avoid_: blocking action-controller work on low-speed smoothness, treating static-friction start failure as PI failure

**Wheel-speed tuning sequence**:
The bench workflow that validates each wheel independently before validating all-wheel commands, so direction, deadband, encoder sign, and per-wheel response problems are isolated before they can hide inside chassis-level behavior.
_Avoid_: tuning all four wheels first, diagnosing chassis motion before wheel-level response

**First-stage speed response standard**:
The coarse bench judgment for wheel-speed tuning that requires stable control dt, correct measured-speed sign, target-direction majority samples, roughly target-sized mean speed, no sustained duty-limit saturation, and clean return to zero after stop.
_Avoid_: per-sample precision, strict low-speed RMS targets, treating encoder quantization noise as physical speed oscillation

**Motion debug mode**:
The explicit human-selected mode for open-loop duty tests or speed-loop bench tests that must not run concurrently with closed-loop action execution.
_Avoid_: compile-time default motion path, hidden motor owner conflict

**Manual calibration commit**:
The workflow where calibration commands print measured values and a human copies accepted constants into source configuration.
_Avoid_: automatic flash save, runtime persistent calibration

**Temporary tuning override**:
The debug text command workflow where closed-loop floating-point gains and limits can be changed at runtime for bench testing, but reset to compile-time defaults after reboot.
_Avoid_: flash persistence, hidden permanent tuning

**Low-speed compensation tuning**:
The temporary bench workflow that exposes static-friction duty and speed feedforward as debug text tuning controls so reliable start can be investigated without enabling compensation by default. Static-friction duty is a bounded start boost: it arms when a nonzero wheel target changes, stays active for a short minimum start window, then drops out after measured wheel speed confirms movement in the target direction or after a maximum boost window.
_Avoid_: hiding static friction inside PI gains, enabling always-on static boost without bench evidence, exposing compensation through the OPENART action protocol

**Speed-loop bench safety envelope**:
The suspended-chassis tuning boundary where AI-assisted serial tests use staged duty and speed limits, multi-second step holds, enforced stop intervals, and immediate stop on abnormal telemetry instead of ground-driving the robot or jumping directly to full power.
_Avoid_: first-test full power, ground motion during wheel-speed tuning, one-second-only stability judgments

**Evidence-backed tuning commit**:
The workflow where a temporary tuning value is copied into source defaults only after bench results record the tested command set, observed speed response, safety state, and reason for accepting that value.
_Avoid_: committing every trial parameter, undocumented tuning defaults

**Speed-loop tuning evidence**:
The paired record of raw serial logs and summarized hardware-test results for each wheel-speed tuning run, including command sequence, target speeds, duration, limits, gains, compensation settings, dt range, speed min/max/mean, saturation, stop behavior, and conclusion.
_Avoid_: tuning by feel only, summary without raw trace, raw trace without decision notes

**Automated speed-loop bench script**:
The repeatable AI-assisted debug workflow that runs speed-loop bench commands, captures raw serial output, computes coarse response statistics, enforces stop behavior, and emits summarized tuning evidence.
_Avoid_: one-off manual serial sessions, unparsed tuning traces, tests that leave motors armed

**Automated action-effect bench script**:
The repeatable AI-assisted debug workflow that injects simulated `MOVE` and `ROTATE` vision decisions, captures action and wheel-speed telemetry, records observed physical action effect, and emits summarized tuning evidence for action-layer parameters.
_Avoid_: wheel-speed-only bench script, visual servoing, tuning by final feel only

**First-pass action bench strata**:
The first-version evidence buckets for the **Automated action-effect bench script**: `MOVE short`, `MOVE long`, `ROTATE 90`, and `ROTATE 180`, with direction-specific `MOVE` splits treated as a follow-up hypothesis only after repeatable asymmetry is observed.
_Avoid_: one undifferentiated action benchmark, assuming per-direction action tuning before evidence

**First-pass move representative distances**:
The first-version `MOVE` evidence points are `20 cm` for `MOVE short` and `100 cm` for `MOVE long`, while `150 cm` remains a later field-coverage target rather than a first-pass benchmark.
_Avoid_: treating every field-length run as a first-pass tuning point, assuming `150 cm` is already inside the default action envelope

**First-pass move direction samples**:
The first-version `MOVE` benchmark runs `up`, `down`, `left`, and `right` as repeated samples inside the same distance bucket so directional asymmetry can be observed without turning direction into a separate parameter class too early.
_Avoid_: benchmarking only one direction, declaring per-direction action tuning before repeatable evidence

**First-pass rotate direction samples**:
The first-version `ROTATE` benchmark runs `cw` and `ccw` as repeated samples inside the same angle bucket so rotation-direction asymmetry can be observed without turning direction into a separate parameter class too early.
_Avoid_: benchmarking only one rotation direction, declaring per-direction action tuning before repeatable evidence

**First-pass action repetition count**:
The first-version benchmark records three runs for every first-pass action condition so tuning decisions rest on repeated evidence rather than a single pass.
_Avoid_: accepting one-off results as representative, treating every condition as a single trial

**First-pass action canonical order**:
The first-version canonical condition order is `MOVE short` `up/down/left/right`, then `MOVE long` `up/down/left/right`, then `ROTATE 90` `cw/ccw`, then `ROTATE 180` `cw/ccw`; full-matrix, subset, and round-robin execution all preserve this order for the conditions they include.
_Avoid_: ad-hoc condition ordering, input-order-dependent subsets, or multiple incompatible "standard" condition sequences

**First-pass action round-robin order**:
The first-version benchmark executes run 1 across all first-pass conditions before run 2, then run 3, so drift from battery, temperature, and floor state is spread across the full evidence set instead of concentrated inside one condition.
_Avoid_: three back-to-back runs of one condition, assuming session drift is negligible

**Structured action-effect observation**:
The per-run manual record of real-world action effect entered immediately after each run in fixed fields: `MOVE` records actual travel, lateral drift, end-heading error, pass/fail from explicit external thresholds, a `good`/`acceptable`/`bad` quality grade, and notes; `ROTATE` records actual angle, translation crosstalk, end-heading error, pass/fail from explicit external thresholds, a `good`/`acceptable`/`bad` quality grade, and notes.
_Avoid_: telemetry-only truth, free-text-only bench notes

**Manual observation sign convention**:
The first-version **Structured action-effect observation** uses command-frame signs and final action-end displacement: `MOVE actual_travel_mm` is the final signed displacement along the commanded axis, `lateral_drift_mm` is the final signed lateral displacement relative to that axis with commanded right positive and left negative, `ROTATE actual_angle_deg` is the final signed net rotation at action end in the commanded protocol sign convention, `end_heading_error_deg` is a signed final-heading error relative to commanded expectation, and `translation_crosstalk_mm` is recorded as unsigned final translation magnitude at action end.
_Avoid_: mixing field coordinates with command coordinates, signed and unsigned drift values without an explicit convention, collapsing heading error sign into magnitude only, mixing final displacement with peak in-run excursion, mixing final axis displacement with total traveled path length, mixing final lateral displacement with peak sideways excursion, mixing final net angle with total accumulated turned angle

**First-pass external action thresholds**:
The first-version `pass/fail` limits for **Structured action-effect observation**: `MOVE short` uses `actual_travel <= +/-10 mm`, `lateral_drift <= 10 mm`, `end_heading_error <= 3 deg`; `MOVE long` uses `actual_travel <= +/-30 mm`, `lateral_drift <= 20 mm`, `end_heading_error <= 5 deg`; `ROTATE 90` uses `actual_angle <= +/-3 deg`, `translation_crosstalk <= 20 mm`, `end_heading_error <= 3 deg`; `ROTATE 180` uses `actual_angle <= +/-5 deg`, `translation_crosstalk <= 30 mm`, `end_heading_error <= 5 deg`.
_Avoid_: pass/fail by impression only, mixing benchmark thresholds with later field-coverage targets

**First-pass hardcoded action bench matrix**:
The first-version action bench script hardcodes its 12 conditions, 3-pass round-robin repetition plan, and first-pass external thresholds instead of introducing a separate configuration file before the workflow is proven.
_Avoid_: premature bench configuration format, rebuilding the same first-pass matrix through external files before the script is stable

**Temporary action tuning override**:
The debug text command workflow that changes action-layer tuning and limits at runtime for bench comparison, then resets to compiled defaults after reboot instead of persisting to flash.
_Avoid_: source edit plus reflash for every bench trial, hidden persistent action tuning

**First-pass action tuning surface**:
The first-version **Temporary action tuning override** exposes only three runtime groups: `move` shape (`max_speed`, `accel`, `kp`, `approach_speed`), `rotate` shape (`max_speed`, `accel`, `kp`, `approach_speed`), and `heading` correction (`kp`, `max_rot`), while completion and fault thresholds stay fixed at first.
_Avoid_: exposing every action threshold on day one, masking control-shape problems by loosening `DONE`, timeout, or obstruction criteria too early

**Idle-only action tuning update**:
The first-version **Temporary action tuning override**, including `action defaults`, may change parameters only while no action is active, and each accepted change applies only to later runs instead of modifying an in-flight evidence sample.
_Avoid_: mid-run action retuning, mixing one evidence run across multiple parameter sets

**Action tuning readback and defaults restore**:
The first-version **Temporary action tuning override** includes explicit `show` readback of the current action-layer runtime tuning using fixed parseable `DATA` lines for move, rotate, and heading groups, and every successful setter or `action defaults` reply also echoes the full current tuning snapshot after one `OK action ...` line.
_Avoid_: hidden active tuning state, reboot-only return to defaults, setter-only debug commands, naming collisions with **Protocol reset**

**Bench-session action tuning state**:
The active action-layer runtime tuning established by **Temporary action tuning override** that persists across single-action completion, `QUERY`, **Emergency stop state**, and **Protocol reset**, and is cleared only by `action defaults` or reboot.
_Avoid_: treating protocol-state reset as tuning reset, one-shot per-action tuning state

**Action debug command module**:
The debug text command Module that owns `action move ...`, `action rotate ...`, `action heading ...`, `action show`, and `action defaults` for **Temporary action tuning override**, without taking over discrete action execution itself.
_Avoid_: mixing action tuning parsing into the top-level command router, treating `action ...` debug commands as the OPENART binary protocol, or coupling the interface layer directly to runtime internals

**Action tuning wrapper APIs**:
The motion-layer wrapper functions exposed through `motion.h` for **Temporary action tuning override**: explicit set-move, set-rotate, set-heading, get-current-tuning, and restore-defaults operations instead of a generic full-config mutator.
_Avoid_: exporting a wide open action-config write surface to the interface layer, bypassing first-pass tuning boundaries

**Mode-tolerant action tuning access**:
The first-version `action show` is available in any motion mode, while `action move`, `action rotate`, `action heading`, and `action defaults` are allowed in any motion mode only when no action is active, because action tuning is a bench-session state rather than an `action_closed_loop`-only transient.
_Avoid_: unnecessarily tying action tuning to one motion mode, allowing mid-action updates under a different name

**Bench-session action initialization**:
The first-version action bench script begins each bench session by sending `action defaults`, then `action show`, and only then applying the trial `action move`/`action rotate`/`action heading` overrides, so every session starts from a provable known tuning state.
_Avoid_: inheriting stale tuning from a prior bench session, assuming reboot or prior script exit already restored defaults

**Per-run protocol reset**:
The first-version action bench script sends `vision sim reset` before each run so the UART action state machine starts from `IDLE` with cleared error and replay state, while **Bench-session action tuning state** stays intact.
_Avoid_: reusing protocol error state across runs, assuming **Protocol reset** should also clear action tuning

**Run completion polling**:
The first-version action bench script treats a run as finished when polled `vision` state leaves `BUSY` for `IDLE` or `ERROR`, then immediately captures final `vision` and `status` snapshots for board-side evidence.
_Avoid_: waiting for a non-existent human-readable `DONE` text line, ending a run without final protocol and action snapshots

**Per-run speed stream window**:
The first-version action bench script enables `stream speed` immediately before each run and turns it off immediately after run completion so wheel-speed telemetry is bounded to that run's evidence window.
_Avoid_: one session-long speed stream that buries prompts and mixes idle text with run telemetry

**Session-level action bench artifacts**:
The first-version action bench stores one raw serial log file and one structured summary file per bench session, with each run identified inside the summary by its own condition, repetition index, tuning snapshot, board-side evidence, and manual observation fields.
_Avoid_: exploding one session into dozens of tiny files, losing per-run identity inside one undifferentiated session note

**Action bench artifact root**:
The first-version action bench stores its session artifacts under a dedicated `.scratch/action-effect-bench/` tree instead of reusing the speed-loop artifact directory.
_Avoid_: mixing action-effect evidence into speed-loop artifact folders, hiding different bench purposes under one log root

**Dual-format action bench summary**:
The first-version action bench writes both a machine-readable session JSON file and a human-readable session Markdown summary for the same bench session, with the Markdown summary containing both per-condition aggregate sections and per-run detail rows.
_Avoid_: machine-only evidence that is hard to review, markdown-only evidence that is hard to compare programmatically, or a flat session summary that is either too high-level or too noisy

**Structured action bench JSON layout**:
The first-version machine-readable session JSON is organized into top-level `session`, `conditions`, and `runs` sections so session metadata, per-condition aggregates, and per-run evidence can be consumed without reconstructing one layer from another.
_Avoid_: a flat JSON dump that forces later tools to reverse-engineer condition summaries from raw runs every time

**Per-condition aggregate summary**:
The first-version condition-level summary is optimized for AI-assisted parameter comparison: it reports `pass_count/total_runs`, quality-grade counts, and manual observation value aggregates using `median`, `min`, and `max`, while board-side final snapshot fields stay lightweight and dense speed-stream evidence remains primarily at run level instead of being aggressively re-aggregated across runs.
_Avoid_: over-interpreting means from three-sample runs, hiding spread when one repeat behaves very differently from the others, or burying AI-useful run evidence inside premature cross-run telemetry statistics

**Low-rate action status timeline**:
The first-version run-level evidence includes a low-rate `status` polling timeline during each action so `DATA action` and `DATA action_feedback` can be observed over time without attempting full control-rate capture.
_Avoid_: relying on final snapshots alone for action-shape diagnosis, or flooding the log with near-control-rate status polling

**Action bench long-term results table**:
The first-version action bench script also appends one session-level summary row to `.scratch/action-effect-bench/hardware-test-results.md`, including `partial` or `aborted` sessions, while detailed evidence stays in the session raw log, JSON, and Markdown files; the table is append-only and later sessions do not rewrite earlier rows.
_Avoid_: manually forgetting to register a completed session, dropping failed sessions from the long-term record, duplicating full per-run detail into the long-term table, or rewriting past evidence to match later interpretations

**Action bench coverage summary**:
The first-version long-term results table records an explicit session coverage summary such as `full-matrix` or a concrete subset description, so session scope is visible without inferring it from status alone.
_Avoid_: guessing tested scope from `primary_status`, hiding subset boundaries inside only the detailed session artifacts

**Action bench quality flag**:
The first-version long-term results table records a lightweight quality flag such as `none` or `concern` alongside the session `primary_status`, so quality concerns remain visible without creating extra primary-status variants; `concern` is triggered by any run graded `bad`, clearly unstable repeat-to-repeat spread inside one condition, or explicitly noted abnormal behavior such as slip, severe oscillation, collision stop, or similar bench-visible anomalies.
_Avoid_: hiding quality concerns inside prose only, overloading the primary status with secondary quality judgment, or inventing an unbounded second scoring system

**Derived action session conclusion**:
The first-version action bench script derives the canonical session-level `primary_status` such as `accepted`, `scoped-pass`, `rejected`, `exploratory`, `partial`, or `aborted` from the recorded evidence and rules, where `accepted` is reserved for standard full-matrix acceptance, `scoped-pass` is reserved for standard subset-only full pass, `rejected` may already follow from any standard-scope failure, reduced-repeat sessions remain `exploratory` without extra pass/fail-flavored substatuses, `partial` means the selected scope was intentionally or manually left incomplete while the evidence chain stayed intact, and `aborted` means the evidence chain broke.
_Avoid_: ad-hoc hand-written primary outcomes for the same evidence, splitting one session conclusion across multiple competing status fields, using full-matrix acceptance language for subset-only evidence, proliferating near-duplicate exploratory status words, or blurring incomplete sessions with broken sessions

**Action bench session ID**:
The first-version action bench uses an `AE01`/`AE02`/... session ID sequence so action-effect evidence never collides with the speed-loop `T01`/`T02`/... records, with automatic next-ID allocation by default and optional manual override for reruns or backfill.
_Avoid_: reusing the speed-loop `Txx` namespace, ambiguous cross-references between different bench families, hard-requiring manual ID bookkeeping

**Action bench session scope**:
The first-version action bench supports both a full-matrix session and a subset session that runs only selected first-pass conditions, while preserving the same per-run evidence shape and session artifacts; subset selection may target either whole first-pass buckets or individual concrete conditions, and selected subsets still execute in the canonical matrix order.
_Avoid_: forcing every tuning iteration through the full 36-run matrix, inventing ad-hoc one-off scripts for partial checks, supporting only one subset granularity, or letting ad-hoc input order change the evidence sequence

**Nonstandard action repetition run**:
An action bench session that intentionally uses fewer than the standard three repeats for selected conditions during exploratory tuning, and is explicitly marked as nonstandard evidence in both session artifacts and the long-term results table.
_Avoid_: silently comparing one-off exploratory runs against standard three-repeat evidence as if they had equal weight

**Action tuning candidate session**:
The first-version action bench treats one session as evidence for exactly one candidate action-tuning set, so every `AE` ID maps to one coherent `move`/`rotate`/`heading` trial parameter set.
_Avoid_: mixing multiple candidate tuning sets inside one session, ambiguous long-term rows that do not correspond to one parameter hypothesis

**Automatic candidate label**:
The first-version action bench derives a stable candidate label automatically from the current `move`/`rotate`/`heading` tuning values only, using normalized value strings that drop redundant trailing zeros and explicit group prefixes such as `m..._r..._h...`, instead of asking for manual naming at session start.
_Avoid_: inconsistent human naming for equivalent parameter sets, extra session input burden for non-essential labels, mixing session environment metadata into candidate identity, generating different labels from numerically equivalent float formatting, or emitting unlabeled bare number sequences that are hard to read

**Session-local candidate judgment**:
The first-version bench treats `primary_status` as a session-local judgment for one candidate under one recorded set of session conditions, and does not auto-merge multiple sessions with the same candidate label into one cross-session verdict.
_Avoid_: treating repeated sessions for one candidate as if they had already been globally reconciled, hiding condition-dependent outcomes behind one premature aggregate label

**Standard candidate acceptance**:
The first-version action bench accepts a candidate tuning set only from a standard three-repeat full-matrix session where every selected condition passes all three repeats; a standard three-repeat subset session may reach `scoped-pass` if its chosen scope fully passes, but any standard-scope failure is enough to mark the candidate `rejected`, while reduced-repeat sessions remain exploratory evidence and do not auto-promote a candidate to accepted.
_Avoid_: accepting a candidate from partial repeat evidence, mixing exploratory quick checks with standard acceptance evidence, turning subjective quality grades into an implicit second pass/fail threshold, treating subset-only coverage as global acceptance, or ignoring a standard-scope failure just because the full matrix was not run

**Action bench failure policy**:
The first-version action bench treats action-level `ERROR` or threshold failure as valid negative evidence for that run while continuing the session, but aborts the whole session on transport or protocol-chain failure such as lost serial control, failed protocol reset, unreadable vision state, or board restart.
_Avoid_: throwing away valid failure evidence, continuing a session after the evidence chain is broken

**Standard session completion rule**:
The first-version standard session continues through all selected conditions even after early run-level failures, unless **Action bench failure policy** triggers a session abort; exploratory subset sessions may still be stopped manually and recorded as `aborted`.
_Avoid_: silently stopping a standard evidence session after the first failed condition, overloading a valid negative result into a transport-level abort

**Bench-only action script responsibility**:
The first-version action bench script is responsible for serial-session control, evidence capture, manual observation prompts, and result artifacts only; build and flash remain external preconditions recorded in the session metadata instead of being driven by the bench script.
_Avoid_: coupling bench evidence generation to build/flash orchestration, hiding flash failures inside action-bench session logic

**Required action session metadata**:
The first-version action bench requires manual `firmware_label`, `flash_status`, `surface_label`, and `power_label` session metadata before running because current board-side debug text does not expose a unique flashed build identity and action-effect evidence depends on floor and power conditions; it does not add automatic git snapshot metadata in the first pass.
_Avoid_: bench evidence that cannot be traced back to a flashed firmware build, comparing sessions across unknown floor or power conditions, assuming startup text already identifies the binary, adding provenance complexity before the bench workflow is stable

**Mac-to-Windows serial bench workflow**:
The current hardware-control workflow where a repository script runs from macOS, uses the configured Windows SSH target and key, and controls the Windows VM serial port such as `COM3` while saving logs back into the shared workspace.
_Avoid_: assuming macOS owns the UART device, ad-hoc VM shell commands without repo scripts

**Fixed-point calibration constants**:
Calibration constants printed and stored as integers with scale suffixes such as `_x100` or physical units such as `_MM`, so humans copy values without mental unit conversion.
_Avoid_: manual divide-by-100, float printf output

**Integer-turn encoder calibration**:
The suspended-wheel calibration command shape where a human rotates one wheel a positive integer number of revolutions and the firmware reports counts per revolution.
_Avoid_: fractional-turn parsing

**IMU X yaw observation**:
The debug telemetry that zeros and streams IMU X-axis angular rate and short-horizon integrated angle to validate the body yaw axis sign.
_Avoid_: trusting gyro Z, skipping axis sign check

**Short-horizon heading**:
The temporary heading estimate used during one local rotation action; it may integrate gyro angular velocity but is not treated as a long-term absolute pose.
_Avoid_: absolute yaw, global heading

**Rotation action feedback**:
The feedback strategy where `ROTATE` uses IMU X-axis short-horizon heading as the primary angle source and encoders only as auxiliary fault evidence.
_Avoid_: encoder-primary rotation angle, long-term yaw accumulation

**Move heading hold**:
The strategy where each `MOVE` action holds the short-horizon heading captured at action start, using IMU X-axis drift only to generate a local `wz` correction.
_Avoid_: global angle tracking during MOVE, distance from IMU

**Body yaw axis**:
The IMU axis that corresponds to horizontal chassis rotation for this physical installation; on this robot, horizontal clockwise/counterclockwise rotation is measured around the IMU X axis.
_Avoid_: assuming gyro Z is yaw

**Body yaw sign**:
The measured IMU X short-horizon heading sign convention where clockwise chassis rotation is positive and counterclockwise chassis rotation is negative.
_Avoid_: assuming CCW-positive math yaw

## Relationships

- The **Upper vision chain** produces **Vision decisions**.
- A **Vision decision** is normally a **Discrete action**.
- **Action response replay** makes repeated **Discrete actions** idempotent across lost `ACK`, `DONE`, or `ERROR` frames.
- UART `STOP` enters **Emergency stop state**, not a normal completed **Discrete action**.
- UART `RESET` performs **Protocol reset**, returning the lower controller to `IDLE` without hardware reboot.
- The **Lower controller** consumes **Discrete actions** and executes them with **Local closed loop** control.
- **Local closed loop** uses encoders for wheel motion feedback and the IMU **Body yaw axis** for rotation feedback.
- The **Closed-loop action runtime** feeds hardware observations into the **Odometry action controller**, applies wheel-speed PI duty output, and hands final action results back to the UART action state machine.
- The **Calibration debug command module** belongs to the **Debug text command layer** and operates **Encoder distance calibration**, not the OPENART binary action protocol.
- The **Motion debug command module** belongs to the **Debug text command layer** and operates the open-loop side of **Motion debug mode**, not closed-loop action execution.
- The **Telemetry debug command module** belongs to the **Debug text command layer** and operates **Calibration telemetry mode**, not the OPENART binary action protocol.
- The **Speed debug command module** belongs to the **Debug text command layer** and operates **Speed-loop bench mode**, not the OPENART binary action protocol.
- The **Action debug command module** belongs to the **Debug text command layer** and operates **Temporary action tuning override**, not the OPENART binary action protocol.
- The **Action debug command module** talks to motion-layer wrapper APIs instead of runtime internals directly.
- The **Action debug command module** uses **Action tuning wrapper APIs** to stay inside the first-pass tuning surface.
- The **Action debug command module** follows **Mode-tolerant action tuning access** across motion modes.
- The **Automated action-effect bench script** begins with **Bench-session action initialization** before applying trial action tuning.
- The **Automated action-effect bench script** performs **Per-run protocol reset** before each action evidence sample.
- The **Automated action-effect bench script** ends each run through **Run completion polling** and final board-side snapshots.
- The **Automated action-effect bench script** uses **Per-run speed stream window** so wheel-speed telemetry stays aligned to one evidence sample.
- The **Automated action-effect bench script** captures **Low-rate action status timeline** during each run for action-shape diagnosis.
- The **Automated action-effect bench script** emits **Session-level action bench artifacts** for one whole bench pass while preserving per-run identity inside the summary.
- **Session-level action bench artifacts** live under **Action bench artifact root**.
- **Session-level action bench artifacts** use **Dual-format action bench summary** for both review and machine comparison.
- **Dual-format action bench summary** uses **Per-condition aggregate summary** before per-run details.
- The **Automated action-effect bench script** updates **Action bench long-term results table** with one lightweight row per completed session.
- **Action bench long-term results table** includes **Action bench coverage summary** for each session row.
- **Action bench long-term results table** includes **Action bench quality flag** alongside each session row.
- The **Automated action-effect bench script** assigns a **Derived action session conclusion** to each session artifact and long-term row.
- The **Automated action-effect bench script** identifies each session with an **Action bench session ID**.
- The **Automated action-effect bench script** may run either **Action bench session scope** as a full matrix or a selected subset.
- The **Automated action-effect bench script** marks exploratory reduced-repeat sessions as **Nonstandard action repetition run**.
- Each **Action bench session ID** corresponds to one **Action tuning candidate session**.
- Each **Action tuning candidate session** carries an **Automatic candidate label** derived from its tuning values.
- Each **Action tuning candidate session** uses **Session-local candidate judgment** rather than automatic cross-session aggregation.
- Candidate acceptance from action bench evidence follows **Standard candidate acceptance**.
- Session continuation versus abort follows **Action bench failure policy**.
- Standard session continuation follows **Standard session completion rule**.
- The **Automated action-effect bench script** follows **Bench-only action script responsibility** and does not own build or flash steps.
- The **Automated action-effect bench script** begins with **Required action session metadata** so each session can be traced to a flashed firmware state.
- The **Vision debug command module** belongs to the **Debug text command layer** and may inject simulated vision frames for bench testing without changing the OPENART binary action protocol.
- **Discrete actions** encode `MOVE` distance in centimeters, but **Local closed loop** converts that value to the **Local motion unit** before planning or control.
- **Local closed loop** derives **Wheel-speed observation** from accumulated encoder totals and uses **Filtered wheel-speed estimate** as the wheel-speed PI control measurement.
- **Local closed loop** uses a **Wheel-speed PI loop** as the first-version inner loop for each wheel.
- The **Odometry action controller** uses **Single-action local odometry**, **Wheel travel calibration**, a **Trapezoid motion profile**, and reports `DONE` only after a **Completion window** is satisfied.
- A `MOVE` **Completion window** uses position error and near-zero wheel speeds; a `ROTATE` **Completion window** uses angle error, near-zero wheel speeds, and near-zero **Body yaw axis** angular rate.
- **Default completion thresholds** are initial tuning values for the **Completion window**, not final competition constants.
- Failed **Discrete actions** report an **Action fault state** over `UART_PROTOCOL.md`.
- **Action fault attribution** decides which **Action fault state** is reported when multiple symptoms overlap.
- The existing mecanum duty solve is validated for the project **Wheel layout** and roller orientation.
- **Forward encoder direction** is the signed encoder convention consumed by odometry and wheel speed loops.
- A **Short-horizon heading** may support one rotation action, but it must not be used as a long-term global pose.
- **Body yaw sign** maps UART `ROTATE CW` to positive short-horizon heading and UART `ROTATE CCW` to negative short-horizon heading.
- **Rotation action feedback** uses the **Body yaw axis** as the primary angle source and encoders as auxiliary fault checks.
- **Move heading hold** uses the **Body yaw axis** only for local yaw correction during a `MOVE`; distance still comes from encoder odometry.
- **Rotation action feedback** and **Move heading hold** may output a **Rotational wheel-speed contribution**, not an angular velocity command.
- **Calibration telemetry mode** precedes closed-loop `MOVE` and `ROTATE` implementation.
- **Calibration telemetry mode** is operated through the **Debug text command layer**, not the OPENART binary action protocol.
- **Speed-loop bench mode** follows calibration and precedes wiring the speed loop into `MOVE` or `ROTATE` actions.
- **First-stage speed-loop acceptance** decides whether the wheel-speed loop is ready to become the inner loop for later `MOVE` and `ROTATE` work.
- **Wheel-speed tuning sequence** checks single-wheel response before all-wheel response.
- **First-stage speed response standard** evaluates bench traces by filtered-speed direction, mean response, saturation, raw-speed spike context, and stop behavior before applying stricter smoothness metrics.
- **Motion debug mode** may reuse motors for bench work, but default firmware motion ownership belongs to closed-loop action execution.
- **Manual calibration commit** is used for first-version encoder calibration constants.
- Closed-loop tuning starts from compile-time defaults and may use **Temporary tuning override** during bench tests.
- **Low-speed compensation tuning** is a **Temporary tuning override** used to investigate reliable low-speed start before any compensation is committed to source defaults.
- **Speed-loop bench safety envelope** bounds AI-assisted tuning before wheel-speed control is used under real chassis motion.
- **Temporary tuning override** becomes an **Evidence-backed tuning commit** only after the bench evidence is recorded and the chosen values are intentionally copied into source defaults.
- **Speed-loop tuning evidence** supports **Evidence-backed tuning commit** decisions.
- The **Automated speed-loop bench script** produces **Speed-loop tuning evidence** for AI-assisted tuning.
- The **Automated action-effect bench script** follows speed-loop acceptance and produces action-layer tuning evidence for **Discrete action** execution.
- The **Automated action-effect bench script** organizes first-pass evidence by **First-pass action bench strata** before any broader action-default commit.
- **First-pass action bench strata** uses **First-pass move representative distances** for its `MOVE short` and `MOVE long` buckets.
- **First-pass move representative distances** use **First-pass move direction samples** inside each `MOVE` distance bucket.
- **First-pass action bench strata** uses **First-pass rotate direction samples** inside its `ROTATE 90` and `ROTATE 180` buckets.
- Each first-pass action condition uses **First-pass action repetition count**.
- **First-pass action repetition count** is scheduled with **First-pass action round-robin order**.
- The **Automated action-effect bench script** records **Structured action-effect observation** for each run in addition to onboard telemetry.
- **Structured action-effect observation** uses **First-pass external action thresholds** to decide first-version `pass/fail`.
- The **Automated action-effect bench script** starts with a **First-pass hardcoded action bench matrix**.
- The **Automated action-effect bench script** uses **Temporary action tuning override** to compare action-layer parameter trials without reflashing each time.
- **Temporary action tuning override** starts with the **First-pass action tuning surface**.
- **Temporary action tuning override** includes **Action tuning readback and defaults restore**.
- **Temporary action tuning override** establishes a **Bench-session action tuning state** across repeated action runs.
- The first **Automated speed-loop bench script** uses the **Mac-to-Windows serial bench workflow** because the UART is currently attached to Windows `COM3`.
- **Fixed-point calibration constants** avoid manual conversion when copying calibration output into source configuration.
- **Integer-turn encoder calibration** is the first-version encoder calibration input method.
- **IMU X yaw observation** is required before using **Rotation action feedback** or **Move heading hold**.

## Example dialogue

> **Dev:** "Can we close the movement loop with OPENART PLUS image feedback?"
> **Domain expert:** "No, the upper vision chain has too much delay. Treat its output as a discrete action, then let the lower controller execute it with local closed loop feedback before accepting the next action."

## Flagged ambiguities

- "yaw" in older IMU code integrates `gyroZ`, but this robot's physical IMU installation maps horizontal chassis rotation to the IMU X axis. Resolved: use **Body yaw axis** for the project term, and do not assume `gyroZ` is yaw.
- Rotation sign can follow mathematical CCW-positive yaw or the measured IMU installation sign. Resolved: **Body yaw sign** is clockwise-positive for this robot, so UART `ROTATE CW` targets positive heading and `ROTATE CCW` targets negative heading.
- "closed loop" can mean visual servoing or local motor control. Resolved: unless otherwise specified, **Local closed loop** means encoder/IMU feedback on the lower controller.
- "control command" can mean continuous velocity streaming or one UART action. Resolved: `UART_PROTOCOL.md` defines **Discrete actions**; OPENART PLUS does not continuously stream control commands.
- Repeated UART frames can mean a retry or a new action. Resolved: the same `SEQ/CMD/DIR/VAL` is **Action response replay**, while the same `SEQ` with different payload is `BAD_CMD`.
- STOP can mean normal cancellation or emergency stop. Resolved: UART `STOP` enters **Emergency stop state** and requires `RESET` before normal actions resume.
- RESET can mean protocol recovery or MCU reboot. Resolved: UART `RESET` is **Protocol reset**, not a hardware reset and not a calibration reset.
- Bench reset can mean clearing protocol state or clearing action tuning state. Resolved: first-pass runs use **Per-run protocol reset** to clear UART action state only; **Bench-session action tuning state** survives until `action defaults` or reboot.
- `MOVE` distance can mean protocol centimeters or local control distance. Resolved: centimeters exist only at the UART protocol boundary; the **Lower controller** uses the **Local motion unit** for closed-loop planning and feedback.
- Wheel speed can mean the latest encoder PIT delta, raw motion-tick speed, or the controller's filtered speed estimate. Resolved: **Wheel-speed observation** is raw speed from accumulated encoder total delta over elapsed motion-control time; **Filtered wheel-speed estimate** is the value consumed by PI, start-boost exit, completion, and fault checks.
- Wheel speed control can mean direct duty mapping, PI, or full PID with feedforward. Resolved: first version is **Wheel-speed PI loop** with D and feedforward fields reserved, static-friction compensation off by default.
- `wz` can mean chassis angular rate or a wheel-speed rotation contribution. Resolved: first-version local control uses **Rotational wheel-speed contribution** and should name code variables `rot_mm_s` or `wz_wheel_mm_s`, not treat them as `deg/s` or `rad/s`.
- "done" can mean the estimated distance reached or the robot physically settled. Resolved: `DONE` requires the **Completion window**, not only target distance crossing.
- "odometry" can mean full global localization or per-action displacement. Resolved: first version uses **Single-action local odometry** only.
- Encoder distance cannot be assumed from theory yet. Resolved: first calibrate encoder counts by manually rotating suspended wheels a known number of revolutions and combining that with measured wheel diameter; ground motion is a later validation step, not the first calibration method.
- Calibration parameters should not collapse all wheels into one encoder scale. Resolved: first version uses per-wheel `counts_per_rev` and one shared measured wheel diameter.
- Wheel IDs are not in front-left-first order. Resolved: **Wheel layout** is wheel 1 front-right, wheel 2 front-left, wheel 3 rear-right, wheel 4 rear-left.
- Rotation angle feedback can come from IMU integration or encoder odometry. Resolved: **Rotation action feedback** is IMU-X-primary for short actions, encoder-assisted for fault detection only.
- "yaw-axis closed loop" can mean rotation feedback, heading hold during movement, or global yaw tracking. Resolved: in the current closed-loop work it includes **Rotation action feedback** and **Move heading hold**, but not long-term yaw accumulation.
- MOVE heading can mean global heading tracking or holding the action's start heading. Resolved: **Move heading hold** keeps the action-start short-horizon heading only.
- `DONE` threshold values are not final tuning results. Resolved: first version starts with **Default completion thresholds** and keeps them configurable.
- Action failures should not collapse into one generic code. Resolved: `UART_PROTOCOL.md` includes `SENSOR_INVALID` and `CONTROL_UNSTABLE` in addition to timeout, obstruction, motor fault, busy, bad command, and bad frame.
- Fault ordering can mean severity or cause attribution. Resolved: use **Action fault attribution** so invalid sensors are not reported as obstruction, clear obstruction need not wait for hard timeout, and unstable control is reserved for valid-sensor non-obstruction failures.
- Closed-loop control must not be tuned before the measurement units are trustworthy. Resolved: implement **Calibration telemetry mode** before enabling closed-loop actions.
- Calibration is for human bench work, not upper-controller action exchange. Resolved: calibration and telemetry commands live in the **Debug text command layer**, while `UART_PROTOCOL.md` remains focused on OPENART discrete actions.
- Speed-loop tuning is bench work, not upper-controller action exchange. Resolved: **Speed-loop bench mode** lives in the **Debug text command layer**, while `UART_PROTOCOL.md` still exposes only discrete actions.
- "wheel-speed loop is done" can mean action-ready inner-loop response or polished low-speed crawl. Resolved: use **First-stage speed-loop acceptance** first, then treat low-speed smoothness as a later refinement.
- Open-loop motor tests can be the default firmware path or a debug mode. Resolved: closed-loop action execution becomes the default motion owner; open-loop and speed-loop testing are explicit **Motion debug mode** choices.
- Calibration output should not introduce runtime persistence yet. Resolved: first version uses **Manual calibration commit**; commands print values and do not save them automatically.
- Tuning commands can mean runtime experimentation or persistent settings. Resolved: first version supports **Temporary tuning override** only; accepted values must be committed to source defaults manually.
- `50 mm/s` start failure can mean PI tuning, motor deadband, or static friction. Resolved: expose **Low-speed compensation tuning** through debug text commands, with compensation defaulting off until bench evidence justifies changing defaults, and make static-friction duty a bounded measured-speed-confirmed start boost so it does not become an always-on `100 mm/s` bias or a 5 ms bang-bang term.
- Encoder speed filtering can mean smoothing odometry or smoothing control input. Resolved: filter only the control-facing **Filtered wheel-speed estimate**; keep encoder delta and **Wheel-speed observation** raw for displacement and diagnostics.
- Speed-loop tuning can be too timid or too risky. Resolved: use the **Speed-loop bench safety envelope** with staged limits, typical `3-5 s` single-wheel holds, `2-4 s` all-wheel holds, stop intervals, and immediate stop on abnormal telemetry.
- Speed-loop logs can mean raw serial dumps or decision summaries. Resolved: **Speed-loop tuning evidence** requires both raw logs and summarized hardware-test results.
- Agent-run speed tests should be repeatable. Resolved: use an **Automated speed-loop bench script** before making repeated tuning comparisons.
- "running effect" can mean wheel-speed response, visual tracking quality, or whole-chassis action behavior. Resolved: the current optimization target is **Automated action-effect bench script** coverage for `MOVE` and `ROTATE` **Discrete actions**, not another wheel-speed-only pass.
- Serial bench scripts can run on the Mac or inside Windows. Resolved: use the **Mac-to-Windows serial bench workflow** for the current setup.
- Action categories can mean runtime parameter classes or just bench evidence buckets. Resolved: first version uses **First-pass action bench strata** as evidence buckets only; runtime gain scheduling is a separate later decision.
- Long `MOVE` distance can mean a first-pass benchmark or a later field requirement. Resolved: first-pass `MOVE long` uses `100 cm`, while `150 cm` is a later field-coverage target.
- `MOVE` directions can be separate parameter classes or repeated evidence samples. Resolved: first version uses **First-pass move direction samples** inside each `MOVE` distance bucket and defers per-direction parameter classes until asymmetry is repeatable.
- `ROTATE` directions can be separate parameter classes or repeated evidence samples. Resolved: first version uses **First-pass rotate direction samples** inside each angle bucket and defers per-direction parameter classes until asymmetry is repeatable.
- Repeated runs can mean optional confirmation or required first-pass evidence. Resolved: first version requires **First-pass action repetition count** of three runs per condition.
- Standard condition order can be implicit, input-driven, or fixed. Resolved: first version uses **First-pass action canonical order** for full and subset execution.
- Repeated runs can be grouped by condition or spread across the whole matrix. Resolved: first version uses **First-pass action round-robin order**.
- Action-effect judgment can mean hard acceptance or softer quality impression. Resolved: first version uses explicit external thresholds for `pass/fail`, while `good`/`acceptable`/`bad` remains a supplementary quality grade.
- External action thresholds can be hand-wavy or fixed per benchmark bucket. Resolved: first version uses **First-pass external action thresholds** for `MOVE short`, `MOVE long`, `ROTATE 90`, and `ROTATE 180`.
- The first bench matrix can be hardcoded or configuration-driven. Resolved: first version uses a **First-pass hardcoded action bench matrix**.
- Runtime tuning can mean wheel-speed-only or action-layer too. Resolved: first version adds **Temporary action tuning override** for the action layer so bench iteration does not require reflashing on every parameter change.
- The first action tuning interface can expose only shape parameters or every completion and safety threshold too. Resolved: first version uses **First-pass action tuning surface** and keeps completion/fault thresholds fixed initially.
- Action tuning support can be setter-only or include inspection and restore commands. Resolved: first version includes **Action tuning readback and defaults restore**.
- Action tuning readback can be human-only prose or fixed parseable output. Resolved: first version uses fixed `DATA` lines for move, rotate, and heading tuning groups.
- Action tuning setters can require a second explicit readback or can echo the full snapshot immediately. Resolved: first version echoes the full current tuning snapshot after each successful setter and `action defaults`.
- Action tuning parsing can stay in `comm.c` or live in its own module. Resolved: first version uses an **Action debug command module** instead of expanding the top-level router inline.
- Action tuning commands can call runtime internals directly or go through motion-layer wrappers. Resolved: first version uses motion-layer wrapper APIs from the **Action debug command module**.
- Action tuning motion APIs can be explicit wrappers or a generic full-config setter/getter. Resolved: first version uses **Action tuning wrapper APIs** with explicit operations for move, rotate, heading, show, and defaults.
- Action tuning access can be locked to `action_closed_loop` mode or follow bench-session semantics across modes. Resolved: first version uses **Mode-tolerant action tuning access**.
- Bench-session tuning can start from inherited state or an explicit known baseline. Resolved: first version uses **Bench-session action initialization** with `action defaults` then `action show` before applying trial overrides.
- "action reset" could mean protocol/state reset or restoring runtime tuning defaults. Resolved: the tuning workflow uses `action defaults`, leaving **Protocol reset** as a separate concept.
- Action tuning restore could be allowed mid-run or only between runs. Resolved: `action defaults` follows **Idle-only action tuning update** and may change only later runs.
- Action-layer runtime tuning could be per-action or a bench-session state. Resolved: first version uses **Bench-session action tuning state** that survives `QUERY`, **Emergency stop state**, **Protocol reset**, and single-action completion until `action defaults` or reboot.
- Run completion can mean waiting for a text `DONE` line or observing protocol state transition. Resolved: first version uses **Run completion polling** on `vision` state and then captures final `vision` plus `status` snapshots.
- Speed telemetry scope can span a whole bench session or one action sample. Resolved: first version uses **Per-run speed stream window** around each run.
- Action-state evidence can be final-snapshot-only or include a low-rate timeline. Resolved: first version captures **Low-rate action status timeline** during each run in addition to final snapshots.
- Bench evidence files can be run-level or session-level. Resolved: first version uses **Session-level action bench artifacts** with per-run structure inside the summary.
- Action bench artifacts can share the speed-loop log root or live in a separate tree. Resolved: first version uses **Action bench artifact root** under `.scratch/action-effect-bench/`.
- Action bench summary can be markdown-only or dual-format. Resolved: first version uses **Dual-format action bench summary** with both JSON and Markdown for the same session.
- Markdown session summary can be flat per-run detail only or layered with aggregate sections. Resolved: first version uses **Dual-format action bench summary** with per-condition aggregates plus per-run details.
- Machine-readable session JSON can be flat or layered by session, condition, and run. Resolved: first version uses **Structured action bench JSON layout** with `session`, `conditions`, and `runs`.
- Condition aggregates can be mean-first or median/spread-first for tiny repeat counts. Resolved: first version uses **Per-condition aggregate summary** with `pass_count`, quality counts, and `median/min/max`.
- Board-side telemetry can be heavily collapsed at condition level or kept rich at run level for later AI tuning. Resolved: first version keeps condition-level telemetry aggregation lightweight and preserves richer speed-stream evidence at run level.
- Long-term action bench evidence can be fully manual or session-auto-registered. Resolved: first version auto-appends one lightweight entry to **Action bench long-term results table** for every completed, partial, or aborted session.
- Long-term evidence tables can rewrite past rows or remain append-only. Resolved: first version keeps **Action bench long-term results table** append-only.
- Long-term rows can omit tested scope or carry an explicit scope summary. Resolved: first version includes **Action bench coverage summary** in each long-term row.
- Quality concern can become a new primary status or remain a secondary flag. Resolved: first version uses **Action bench quality flag** as a separate field instead of expanding `primary_status`.
- Quality concern can be left as vague prose or triggered by explicit evidence patterns. Resolved: first version raises **Action bench quality flag** from `bad` run grades, unstable repeat spread, or explicitly noted abnormal bench behavior.
- Bench session IDs can share the speed-loop `Txx` sequence or use a dedicated namespace. Resolved: first version uses **Action bench session ID** with an `AExx` prefix.
- Session conclusions can be hand-authored or derived from the evidence rules. Resolved: first version uses **Derived action session conclusion** as the canonical `primary_status`, with any explanation kept inline in the session summary or long-term note.
- Incomplete sessions can reflect an intact but intentionally stopped run set or a broken evidence chain. Resolved: first version uses `partial` for the former and `aborted` for the latter under **Derived action session conclusion**.
- Session IDs can be always manual or auto-assigned with override. Resolved: first version auto-assigns the next **Action bench session ID** by default and allows manual override when needed.
- Action bench sessions can be full-matrix-only or allow targeted subsets. Resolved: first version supports **Action bench session scope** for both full-matrix and subset sessions.
- Subset selection can be bucket-only or condition-level too. Resolved: first version lets **Action bench session scope** target whole buckets or individual concrete conditions.
- Subset all-pass evidence can reuse `accepted` or carry a scope-limited status. Resolved: first version uses `scoped-pass` for standard subset sessions that fully pass within their chosen scope.
- Subset standard failures can stay scope-limited or globally reject the candidate. Resolved: first version treats any standard-scope failure as `rejected`, even when the tested scope is a subset.
- Subset execution can follow ad-hoc input order or canonical matrix order. Resolved: first version executes subset selections in canonical matrix order.
- Repetition count can stay standard or be reduced for exploratory sessions. Resolved: first version allows **Nonstandard action repetition run** with explicit marking whenever repeats are fewer than three.
- Reduced-repeat sessions can invent extra success/failure-flavored substatuses or remain under one exploratory state. Resolved: first version keeps all reduced-repeat sessions under `exploratory`.
- One session can represent one candidate tuning set or mix several. Resolved: first version uses **Action tuning candidate session** so one `AE` session maps to exactly one candidate action parameter set.
- Candidate labels can be manual or automatically derived from tuning values. Resolved: first version uses **Automatic candidate label** and does not ask for manual candidate naming.
- Candidate labels can reflect only tuning values or also session environment metadata. Resolved: first version binds **Automatic candidate label** to action tuning values only.
- Candidate labels can preserve raw float formatting or normalize equivalent numeric strings. Resolved: first version normalizes numeric formatting inside **Automatic candidate label** by dropping redundant trailing zeros.
- Candidate labels can be bare numeric tuples or prefixed by tuning-group identity. Resolved: first version formats **Automatic candidate label** with explicit `m`/`r`/`h` group prefixes.
- Same-label candidate sessions can auto-collapse into one verdict or remain separate per session. Resolved: first version uses **Session-local candidate judgment** and does not auto-merge repeated sessions for the same candidate label.
- Candidate evidence can be exploratory-only or strong enough for acceptance. Resolved: first version uses **Standard candidate acceptance** and does not auto-accept from reduced-repeat sessions.
- Quality grades can be diagnostic notes or a second hard acceptance gate. Resolved: first version keeps `good`/`acceptable`/`bad` diagnostic under **Standard candidate acceptance** and does not let them replace explicit `pass/fail`.
- Bench failure can mean a valid run-level negative result or a broken evidence chain. Resolved: first version uses **Action bench failure policy** to continue through action-level failures but abort on transport or protocol-chain failure.
- A standard session can stop at first failure or continue to complete the selected matrix. Resolved: first version uses **Standard session completion rule** and continues unless the session abort policy triggers.
- Manual action observations can use field axes or command-frame axes. Resolved: first version uses **Manual observation sign convention** for all structured per-run entries.
- Bench automation can include build/flash orchestration or only the serial evidence loop. Resolved: first version uses **Bench-only action script responsibility** and treats build/flash as external recorded preconditions.
- Flashed firmware identity can come from board-side text or required session metadata. Resolved: first version uses **Required action session metadata** because current startup text does not uniquely identify the flashed build.
- Session provenance can stay manual in first pass or include automatic git snapshot metadata. Resolved: first version keeps provenance manual inside **Required action session metadata** and skips automatic git snapshot capture.
- Action-effect comparability can ignore or record floor condition. Resolved: first version includes `surface_label` inside **Required action session metadata** because floor condition materially affects action evidence.
- Action-effect comparability can ignore or record power condition. Resolved: first version includes `power_label` inside **Required action session metadata** because supply state materially affects action evidence.
- Manual action-effect notes can mean subjective remarks or comparable measurements. Resolved: first version records **Structured action-effect observation** rather than notes alone.
- Manual action-effect entry can mean per-run or per-round. Resolved: first version records **Structured action-effect observation** immediately after each run, not after a full 12-condition round.
- Manual action-effect judgment can be only pass/fail or include a comparable quality label. Resolved: first version keeps `pass/fail` and also records one `good`/`acceptable`/`bad` quality grade inside **Structured action-effect observation**.
- Move travel can mean final axis displacement or total path length. Resolved: first version stores final commanded-axis displacement in `actual_travel_mm` under **Manual observation sign convention**.
- Lateral drift can mean final sideways displacement or peak sideways excursion. Resolved: first version stores final commanded-frame lateral displacement in `lateral_drift_mm`.
- Rotate angle can mean final net rotation or total accumulated turned angle. Resolved: first version stores final signed net rotation at action end in `actual_angle_deg`.
- End-heading error can be stored as magnitude only or as a signed bias. Resolved: first version stores signed `end_heading_error_deg` under **Manual observation sign convention**, while threshold checks use its absolute value.
- Rotate translation crosstalk can mean final displacement or peak in-run excursion. Resolved: first version stores final action-end displacement magnitude in `translation_crosstalk_mm`.
- AI-assisted tuning may run serial tests and temporary tuning overrides, but source defaults should change only through an **Evidence-backed tuning commit**.
- Closed-loop tuning values can be fixed-point integers or human-readable floating-point values. Resolved: PID gains and limits use floating-point values, while calibration constants keep their explicit fixed-point or physical-unit suffixes.
- Fixed-point output must not force humans to do arithmetic before editing config. Resolved: print and store scaled integer constants using the same suffix, such as `COUNTS_PER_REV_X100`.
- Encoder calibration input should stay simple. Resolved: first version accepts positive integer wheel turns only.
- IMU body yaw sign must be measured on the physical robot. Resolved: first version includes **IMU X yaw observation** before closed-loop rotation tuning.
