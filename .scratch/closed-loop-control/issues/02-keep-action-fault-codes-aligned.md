# Keep action fault codes aligned

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Keep the OPENART UART protocol documentation and firmware constants aligned for action fault states used by the future odometry action controller.

The protocol should distinguish timeout, obstruction, invalid sensor feedback, unstable control, motor fault, busy, bad command, and bad frame instead of collapsing closed-loop failures into a generic motor fault.

## Acceptance criteria

- [x] `UART_PROTOCOL.md` documents `SENSOR_INVALID = 0x07` and `CONTROL_UNSTABLE = 0x08`.
- [x] Firmware protocol constants define `VISION_PROTOCOL_ERROR_SENSOR_INVALID` and `VISION_PROTOCOL_ERROR_CONTROL_UNSTABLE`.
- [x] `UART_PROTOCOL.md` explains the boundary between `TIMEOUT`, `OBSTRUCTED`, `SENSOR_INVALID`, `CONTROL_UNSTABLE`, and `MOTOR_FAULT`.
- [x] The protocol version history records the fault-code extension.
- [x] Existing command values and existing error values remain unchanged for backward compatibility.

## Blocked by

None - can start immediately.

## Comments

- Verified with native contract test: `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/app/module/perception -IProjects/user/common tests/native/test_uart_protocol_contract.c -o /tmp/test_uart_protocol_contract && /tmp/test_uart_protocol_contract`
- Verified with `./tools/keil-vscode.sh build` -> `0 Error(s), 0 Warning(s)`
