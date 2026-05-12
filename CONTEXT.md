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

**Single-action local odometry**:
The encoder-derived displacement estimate reset at the start of one discrete action and discarded after `DONE` or `ERROR`.
_Avoid_: global pose, long-term localization

**Local motion unit**:
The lower-controller convention that local distance, displacement, and wheel-speed control use millimeters and millimeters per second after converting UART `MOVE` centimeters at the protocol boundary.
_Avoid_: centimeter control internals, raw encoder-count control units

**Wheel-speed observation**:
The measured wheel linear speed calculated in the motion tick from accumulated encoder total delta over elapsed control time.
_Avoid_: latest 5 ms encoder sample as the control speed, raw count-per-tick speed

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

**Speed-loop bench mode**:
The human-operated debug mode that commands wheel-speed targets and streams target speed, measured speed, duty, encoder delta, and control dt before closed-loop actions use the speed loop.
_Avoid_: OPENART speed command, direct competition action execution

**Motion debug mode**:
The explicit human-selected mode for open-loop duty tests or speed-loop bench tests that must not run concurrently with closed-loop action execution.
_Avoid_: compile-time default motion path, hidden motor owner conflict

**Manual calibration commit**:
The workflow where calibration commands print measured values and a human copies accepted constants into source configuration.
_Avoid_: automatic flash save, runtime persistent calibration

**Temporary tuning override**:
The debug text command workflow where closed-loop floating-point gains and limits can be changed at runtime for bench testing, but reset to compile-time defaults after reboot.
_Avoid_: flash persistence, hidden permanent tuning

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
- **Discrete actions** encode `MOVE` distance in centimeters, but **Local closed loop** converts that value to the **Local motion unit** before planning or control.
- **Local closed loop** uses **Wheel-speed observation** for the wheel speed loop rather than treating the latest encoder interrupt sample as the control measurement.
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
- **Motion debug mode** may reuse motors for bench work, but default firmware motion ownership belongs to closed-loop action execution.
- **Manual calibration commit** is used for first-version encoder calibration constants.
- Closed-loop tuning starts from compile-time defaults and may use **Temporary tuning override** during bench tests.
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
- `MOVE` distance can mean protocol centimeters or local control distance. Resolved: centimeters exist only at the UART protocol boundary; the **Lower controller** uses the **Local motion unit** for closed-loop planning and feedback.
- Wheel speed can mean the latest encoder PIT delta or the controller's speed estimate. Resolved: **Wheel-speed observation** is computed from accumulated encoder total delta over elapsed motion-control time.
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
- Open-loop motor tests can be the default firmware path or a debug mode. Resolved: closed-loop action execution becomes the default motion owner; open-loop and speed-loop testing are explicit **Motion debug mode** choices.
- Calibration output should not introduce runtime persistence yet. Resolved: first version uses **Manual calibration commit**; commands print values and do not save them automatically.
- Tuning commands can mean runtime experimentation or persistent settings. Resolved: first version supports **Temporary tuning override** only; accepted values must be committed to source defaults manually.
- Closed-loop tuning values can be fixed-point integers or human-readable floating-point values. Resolved: PID gains and limits use floating-point values, while calibration constants keep their explicit fixed-point or physical-unit suffixes.
- Fixed-point output must not force humans to do arithmetic before editing config. Resolved: print and store scaled integer constants using the same suffix, such as `COUNTS_PER_REV_X100`.
- Encoder calibration input should stay simple. Resolved: first version accepts positive integer wheel turns only.
- IMU body yaw sign must be measured on the physical robot. Resolved: first version includes **IMU X yaw observation** before closed-loop rotation tuning.
