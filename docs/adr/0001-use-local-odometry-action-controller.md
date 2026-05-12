# Use a local odometry action controller

Accepted. The upper vision chain sends discrete `MOVE` and `ROTATE` actions over `UART_PROTOCOL.md`, and the lower controller executes each action locally with encoder odometry, IMU short-horizon heading, speed control, trapezoid motion profiles, completion windows, and explicit fault states. We reject both fixed-speed/fixed-time execution and realtime camera visual servoing because the competition pipeline has multiple vision and communication delays; local encoder/IMU feedback is the only feedback fast enough to decide `DONE` accurately.

## Consequences

- `MOVE` uses encoder-derived chassis displacement as the distance source, four wheel speed loops as the inner loop, and IMU heading hold only for orientation correction.
- `MOVE` heading hold keeps the action-start short-horizon heading; it does not track or update a long-term global angle.
- The first version uses single-action local odometry only: reset displacement at action start, estimate progress along `UP/DOWN/LEFT/RIGHT`, and discard the estimate after `DONE` or `ERROR`.
- Encoder distance calibration is required before distance closed loop can be trusted; first measure encoder counts per wheel revolution for each suspended wheel and combine those values with one shared measured wheel diameter, then use ground motion only as a validation step.
- `ROTATE` uses the project **Body yaw axis** for short-horizon heading feedback as the primary angle source, with encoders used as auxiliary fault evidence rather than the primary angle source.
- `DONE` is reported only after target error is small, wheel speed is near zero, and the state is stable for a short completion window. First-version thresholds are 1.0 cm for `MOVE`, 2 degrees for `ROTATE`, and 100 ms of stability, kept as tuning parameters.
- Faults distinguish timeout, obstruction, invalid sensor feedback, unstable control, and motor faults instead of collapsing all failures into a generic motor fault.
- Implementation starts with calibration and telemetry mode so encoder and IMU units are trustworthy before PID tuning or closed-loop action execution.
- Calibration and telemetry are exposed through the debug text command layer in `comm.c`; the OPENART binary UART protocol remains focused on discrete competition actions.
- Calibration commands print results only; accepted constants are manually committed to source configuration rather than saved to flash at runtime.
- Calibration output and source configuration use the same fixed-point integer scale, such as `COUNTS_PER_REV_X100`, so accepted values can be copied directly without manual conversion.
- Encoder calibration accepts positive integer wheel turns only in the first version.
- The calibration phase includes IMU X yaw observation to validate axis sign before `ROTATE` or `MOVE` heading hold tuning.
