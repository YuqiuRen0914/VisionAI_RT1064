# Add closed-loop action state machine

Status: done
Type: enhancement

## Parent

`.scratch/closed-loop-control/PRD.md`

## What to build

Replace the open-loop test as the default motor owner with a closed-loop action state machine shell for OPENART discrete actions. The shell should own `MOVE`, `ROTATE`, `STOP`, `QUERY`, and `RESET` lifecycles even before the full movement controllers are complete.

This slice should implement the protocol recovery behavior agreed in `UART_PROTOCOL.md`: action response replay, emergency stop state, protocol reset, busy rejection, bad command validation, and status reporting.

## Acceptance criteria

- [x] Default firmware motion ownership belongs to closed-loop action execution; open-loop duty testing and speed-loop bench testing are explicit debug modes.
- [x] A valid `MOVE` or `ROTATE` enters a busy action state and returns `ACK` within the protocol window.
- [x] Invalid command parameters return `ERROR / BAD_CMD` without starting motion.
- [x] A second `MOVE` or `ROTATE` while an action is active returns `ERROR / BUSY` unless it is an idempotent retry of the same action frame.
- [x] Repeating the same `SEQ/CMD/DIR/VAL` while active replays `ACK` and does not restart the action.
- [x] Repeating the same `SEQ/CMD/DIR/VAL` after completion replays the prior final `DONE` or `ERROR`.
- [x] Reusing the same `SEQ` with different `CMD/DIR/VAL` returns `ERROR / BAD_CMD` and does not start a new action.
- [x] `STOP` immediately commands zero duty, interrupts the active action, returns `ACK`, and enters `ERROR` status until `RESET`.
- [x] `RESET` stops motors, clears error state, clears current action state, clears action response replay memory, and returns the controller to `IDLE` without rebooting the MCU or changing calibration constants.
- [x] `QUERY` returns `STATUS / IDLE`, `STATUS / BUSY`, or `STATUS / ERROR` consistently with the state machine.
- [x] Native protocol/action tests cover replay, stop, reset, busy, bad command, and query semantics.
- [x] Keil build succeeds.

## Blocked by

None - can start immediately.

## Comments

- Implemented with TDD through `tests/native/test_action_controller.c`.
- Verified native test: `cc -std=c99 -Wall -Wextra -Werror -IProjects/user/common -IProjects/user/app/module/perception -IProjects/user/app/module/motion tests/native/test_action_controller.c Projects/user/app/module/perception/action_controller.c -o /tmp/test_action_controller && /tmp/test_action_controller`
- Verified existing protocol contract test still passes.
- Verified integration build: `./tools/keil-vscode.sh build` -> `0 Error(s), 0 Warning(s)`.
