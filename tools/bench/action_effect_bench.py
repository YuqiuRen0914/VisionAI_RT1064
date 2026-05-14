#!/usr/bin/env python3
"""Run one-condition action-effect bench sessions through the Windows COM port."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ARTIFACT_ROOT = REPO_ROOT / ".scratch" / "action-effect-bench"
DEFAULT_HOST = "Windows@10.211.55.3"
DEFAULT_PORT = "COM3"
DEFAULT_BAUD = 115200
DEFAULT_SSH_KEY = Path.home() / ".ssh" / "keil_windows_ed25519"
MAC_DOCUMENTS_PREFIX = Path("/Volumes/[C] Windows 11/Users/jjp/Documents")
WINDOWS_DOCUMENTS_PREFIX = r"C:\Users\jjp\Documents"

VISION_STATUS_RE = re.compile(r"OK vision status=(?P<status>\d+)")
RUN_END_RE = re.compile(r"### ACTION_RUN_END\b.*\bfinal=(?P<final>\S+)")


@dataclass(frozen=True)
class ActionCondition:
    id: str
    action: str
    bucket: str
    direction: str
    value: int
    command: str


@dataclass(frozen=True)
class RemoteResult:
    returncode: int
    stdout: str
    stderr: str = ""


def ps_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def fmt_float(value: float) -> str:
    if abs(value - round(value)) < 0.0001:
        return str(int(round(value)))
    return f"{value:.3f}".rstrip("0").rstrip(".")


def parse_tuning_values(text: str, expected_count: int, label: str) -> tuple[float, ...]:
    pieces = text.split()
    if len(pieces) != expected_count:
        raise argparse.ArgumentTypeError(f"{label} expects {expected_count} numeric values")
    try:
        return tuple(float(piece) for piece in pieces)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"{label} expects numeric values") from exc


def format_tuning_command(group: str, values: tuple[float, ...]) -> str:
    return f"action {group} " + " ".join(fmt_float(value) for value in values)


def build_condition(action: str, direction: str, value: int) -> ActionCondition:
    if action == "move":
        if direction not in {"up", "down", "left", "right"}:
            raise argparse.ArgumentTypeError("MOVE direction must be up, down, left, or right")
        if value <= 0 or value > 255:
            raise argparse.ArgumentTypeError("MOVE value must be 1..255 cm")
        bucket = "short" if value <= 20 else "long"
        condition_id = f"move_{bucket}_{direction}"
        return ActionCondition(
            id=condition_id,
            action=action,
            bucket=f"MOVE {bucket}",
            direction=direction,
            value=value,
            command=f"vision sim move {direction} {value}",
        )

    if direction not in {"cw", "ccw"}:
        raise argparse.ArgumentTypeError("ROTATE direction must be cw or ccw")
    if value not in {90, 180}:
        raise argparse.ArgumentTypeError("ROTATE value must be 90 or 180 degrees")
    condition_id = f"rotate_{value}_{direction}"
    return ActionCondition(
        id=condition_id,
        action=action,
        bucket=f"ROTATE {value}",
        direction=direction,
        value=value,
        command=f"vision sim rotate {direction} {value}",
    )


def parse_vision_status(text: str) -> str:
    run_end_matches = list(RUN_END_RE.finditer(text))
    if run_end_matches:
        return run_end_matches[-1].group("final")

    matches = list(VISION_STATUS_RE.finditer(text))
    if not matches:
        return "UNKNOWN"
    status = matches[-1].group("status")
    if status == "0":
        return "IDLE"
    if status == "1":
        return "BUSY"
    if status == "2":
        return "ERROR"
    return status


def build_remote_script(
    args: argparse.Namespace,
    condition: ActionCondition,
    run_index: int,
    setup_commands: list[str],
) -> str:
    setup_blocks = [f"Send-Line {ps_quote(command)} 0.250" for command in setup_commands]
    setup_text = "\n  ".join(setup_blocks)

    return f"""
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$portName = {ps_quote(args.port)}
$baud = {args.baud}
$sp = New-Object System.IO.Ports.SerialPort $portName, $baud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$sp.NewLine = "`n"
$sp.ReadTimeout = 100
$sp.WriteTimeout = 500

function Emit([string]$text) {{
  [Console]::Out.WriteLine($text)
}}

function EmitRaw([string]$text) {{
  [Console]::Out.Write($text)
}}

function Read-For([double]$seconds) {{
  $deadline = (Get-Date).AddMilliseconds([int]($seconds * 1000))
  $sb = New-Object System.Text.StringBuilder
  while ((Get-Date) -lt $deadline) {{
    $s = $sp.ReadExisting()
    if ($s.Length -gt 0) {{ [void]$sb.Append($s) }}
    Start-Sleep -Milliseconds 20
  }}
  if ($sb.Length -gt 0) {{ EmitRaw ($sb.ToString().Replace("`r", "")) }}
  return $sb.ToString().Replace("`r", "")
}}

function Send-Line([string]$cmd, [double]$waitSeconds) {{
  Emit ">>> $cmd"
  $sp.WriteLine($cmd)
  Start-Sleep -Milliseconds 40
  return Read-For $waitSeconds
}}

function Poll-Vision-Until-Done([double]$timeoutSeconds) {{
  $deadline = (Get-Date).AddMilliseconds([int]($timeoutSeconds * 1000))
  $final = "UNKNOWN"
  while ((Get-Date) -lt $deadline) {{
    $text = Send-Line 'vision' 0.120
    if ($text -match 'OK vision status=([0-9]+)') {{
      $status = $Matches[1]
      if ($status -ne '1') {{
        if ($status -eq '0') {{ $final = 'IDLE' }}
        elseif ($status -eq '2') {{ $final = 'ERROR' }}
        else {{ $final = $status }}
        return $final
      }}
    }}
    Send-Line 'status' 0.080 | Out-Null
    Start-Sleep -Milliseconds {int(args.poll_interval_ms)}
  }}
  return 'TIMEOUT'
}}

$sp.Open()
try {{
  Start-Sleep -Milliseconds 200
  $sp.DiscardInBuffer()
  Emit "### ACTION_BENCH_BEGIN session={args.session_id} port=$portName baud=$baud"
  {setup_text}
  Emit "### ACTION_RUN_BEGIN run={run_index} condition={condition.id}"
  Send-Line 'vision sim reset' 0.300 | Out-Null
  Send-Line 'stream speed' 0.300 | Out-Null
  Send-Line {ps_quote(condition.command)} 0.300 | Out-Null
  $final = Poll-Vision-Until-Done {args.run_timeout_s:.3f}
  Send-Line 'stream off' 0.300 | Out-Null
  Send-Line 'vision' 0.300 | Out-Null
  Send-Line 'status' 0.300 | Out-Null
  Emit "### ACTION_RUN_END run={run_index} condition={condition.id} final=$final"
  Emit "### ACTION_BENCH_END session={args.session_id}"
}}
finally {{
  if ($sp.IsOpen) {{
    try {{
      $sp.WriteLine('stream off')
      Start-Sleep -Milliseconds 50
      $tail = $sp.ReadExisting()
      if ($tail.Length -gt 0) {{ EmitRaw ($tail.Replace("`r", "")) }}
    }} catch {{}}
    $sp.Close()
  }}
}}
""".lstrip()


def prompt_choice(prompt: str, choices: set[str]) -> str:
    while True:
        value = input(prompt).strip().lower()
        if value in choices:
            return value
        print(f"Expected one of: {', '.join(sorted(choices))}")


def prompt_float(prompt: str) -> float:
    while True:
        value = input(prompt).strip()
        try:
            return float(value)
        except ValueError:
            print("Expected a numeric value.")


def prompt_observation(condition: ActionCondition) -> dict[str, object]:
    print("Structured action-effect observation")
    if condition.action == "move":
        observation: dict[str, object] = {
            "actual_travel_mm": prompt_float("actual_travel_mm: "),
            "lateral_drift_mm": prompt_float("lateral_drift_mm: "),
            "end_heading_error_deg": prompt_float("end_heading_error_deg: "),
        }
    else:
        observation = {
            "actual_angle_deg": prompt_float("actual_angle_deg: "),
            "translation_crosstalk_mm": prompt_float("translation_crosstalk_mm: "),
            "end_heading_error_deg": prompt_float("end_heading_error_deg: "),
        }
    observation["pass_fail"] = prompt_choice("pass_fail [pass/fail]: ", {"pass", "fail"})
    observation["quality"] = prompt_choice(
        "quality [good/acceptable/bad]: ", {"good", "acceptable", "bad"}
    )
    observation["notes"] = input("notes: ").strip()
    return observation


def run_setup_commands(args: argparse.Namespace) -> list[str]:
    commands = ["stream off", "action defaults", "action show"]
    if args.action_move is not None:
        commands.append(format_tuning_command("move", args.action_move))
    if args.action_rotate is not None:
        commands.append(format_tuning_command("rotate", args.action_rotate))
    if args.action_heading is not None:
        commands.append(format_tuning_command("heading", args.action_heading))
    return commands


def mac_path_to_windows(path: Path) -> str:
    resolved = path.resolve()
    try:
        relative = resolved.relative_to(MAC_DOCUMENTS_PREFIX)
    except ValueError as exc:
        raise ValueError(f"path is not under Windows shared Documents: {resolved}") from exc
    return WINDOWS_DOCUMENTS_PREFIX + "\\" + "\\".join(relative.parts)


def windows_double_quote(value: str) -> str:
    return '"' + value.replace('"', '\\"') + '"'


def run_remote(args: argparse.Namespace, script: str) -> RemoteResult:
    script_path = args.artifact_root / "sessions" / args.session_id / "_action-effect-current.ps1"
    script_path.parent.mkdir(parents=True, exist_ok=True)
    script_path.write_text(script, encoding="utf-8")
    windows_script_path = mac_path_to_windows(script_path)
    command = [
        "ssh",
        "-i",
        str(args.ssh_key),
        "-o",
        "IdentitiesOnly=yes",
        "-o",
        "PubkeyAuthentication=unbound",
        "-o",
        "ConnectTimeout=8",
        args.host,
        (
            "powershell.exe -NoProfile -NonInteractive -OutputFormat Text "
            f"-ExecutionPolicy Bypass -File {windows_double_quote(windows_script_path)}"
        ),
    ]
    try:
        completed = subprocess.run(
            command,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            check=False,
        )
        return RemoteResult(completed.returncode, completed.stdout, completed.stderr)
    finally:
        try:
            script_path.unlink()
        except OSError:
            pass


def make_condition_artifact(condition: ActionCondition, run_count: int) -> dict[str, object]:
    return {
        "id": condition.id,
        "action": condition.action,
        "bucket": condition.bucket,
        "direction": condition.direction,
        "value": condition.value,
        "selected_runs": run_count,
    }


def make_summary(artifact: dict[str, object], raw_log_path: Path, json_path: Path) -> str:
    session = artifact["session"]
    condition = artifact["conditions"][0]
    runs = artifact["runs"]
    lines = [
        "# Action-Effect Bench Summary",
        "",
        f"- Session: `{session['id']}`",
        f"- Candidate: `{session['candidate_label']}`",
        f"- Firmware: `{session['firmware_label']}`",
        f"- Flash status: `{session['flash_status']}`",
        f"- Surface: `{session['surface_label']}`",
        f"- Power: `{session['power_label']}`",
        f"- Build/flash policy: `{session['build_flash_policy']}`",
        f"- Raw log: `{raw_log_path}`",
        f"- JSON: `{json_path}`",
        "",
        "## Condition",
        "",
        f"- ID: `{condition['id']}`",
        f"- Command: `{runs[0]['command'] if runs else ''}`",
        "",
        "## Structured Action-Effect Observation",
        "",
        "| Run | Final vision | Pass/fail | Quality | Observation | Notes |",
        "| --- | ------------ | --------- | ------- | ----------- | ----- |",
    ]

    for run in runs:
        observation = run["observation"]
        observation_text = ", ".join(
            f"{key}={value}" for key, value in observation.items() if key not in {"notes"}
        )
        lines.append(
            "| "
            + " | ".join(
                [
                    str(run["run_index"]),
                    f"`{run['final_vision_state']}`",
                    str(observation.get("pass_fail", "")),
                    str(observation.get("quality", "")),
                    observation_text,
                    str(observation.get("notes", "")),
                ]
            )
            + " |"
        )

    lines.append("")
    return "\n".join(lines)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=os.environ.get("KEIL_SSH_HOST", DEFAULT_HOST))
    parser.add_argument("--ssh-key", type=Path, default=DEFAULT_SSH_KEY)
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--artifact-root", type=Path, default=DEFAULT_ARTIFACT_ROOT)
    parser.add_argument("--session-id", required=True)
    parser.add_argument("--candidate-label", default="")
    parser.add_argument("--firmware-label", required=True)
    parser.add_argument("--flash-status", required=True)
    parser.add_argument("--surface-label", required=True)
    parser.add_argument("--power-label", required=True)
    parser.add_argument("--action", choices=["move", "rotate"], required=True)
    parser.add_argument("--direction", required=True)
    parser.add_argument("--value", type=int, required=True)
    parser.add_argument("--runs", type=int, default=1)
    parser.add_argument("--run-timeout-s", type=float, default=20.0)
    parser.add_argument("--poll-interval-ms", type=int, default=100)
    parser.add_argument(
        "--action-move",
        type=lambda text: parse_tuning_values(text, 4, "--action-move"),
    )
    parser.add_argument(
        "--action-rotate",
        type=lambda text: parse_tuning_values(text, 4, "--action-rotate"),
    )
    parser.add_argument(
        "--action-heading",
        type=lambda text: parse_tuning_values(text, 2, "--action-heading"),
    )
    parser.add_argument("--plan-only", action="store_true")
    args = parser.parse_args(argv)

    if args.runs <= 0:
        parser.error("--runs must be > 0")
    if args.run_timeout_s <= 0.0:
        parser.error("--run-timeout-s must be > 0")
    if args.poll_interval_ms <= 0:
        parser.error("--poll-interval-ms must be > 0")

    try:
        args.condition = build_condition(args.action, args.direction, args.value)
    except argparse.ArgumentTypeError as exc:
        parser.error(str(exc))

    if not args.candidate_label:
        args.candidate_label = "candidate"

    return args


def main(
    argv: list[str],
    remote_runner: Callable[[argparse.Namespace, str], RemoteResult] | None = None,
) -> int:
    args = parse_args(argv)
    remote_runner = remote_runner or run_remote
    condition: ActionCondition = args.condition
    setup_commands = run_setup_commands(args)

    if args.plan_only:
        print("# Action-effect bench plan")
        print(f"session={args.session_id} condition={condition.id} runs={args.runs}")
        for command in setup_commands:
            print(command)
        for run_index in range(1, args.runs + 1):
            print(f"run {run_index}: vision sim reset")
            print(f"run {run_index}: stream speed")
            print(f"run {run_index}: {condition.command}")
            print(f"run {run_index}: poll vision until not BUSY")
            print(f"run {run_index}: stream off")
            print(f"run {run_index}: vision")
            print(f"run {run_index}: status")
        return 0

    started_at = dt.datetime.now().astimezone()
    session_dir = args.artifact_root / "sessions" / args.session_id
    session_dir.mkdir(parents=True, exist_ok=True)
    raw_log_path = session_dir / f"{args.session_id}.raw.log"
    json_path = session_dir / f"{args.session_id}.json"
    summary_path = session_dir / f"{args.session_id}.summary.md"

    raw_chunks: list[str] = []
    runs: list[dict[str, object]] = []
    exit_code = 0

    for run_index in range(1, args.runs + 1):
        per_run_setup = setup_commands if run_index == 1 else []
        script = build_remote_script(args, condition, run_index, per_run_setup)
        result = remote_runner(args, script)
        raw_text = result.stdout
        if result.stderr:
            raw_text += "\n### STDERR\n" + result.stderr
        raw_chunks.append(raw_text)

        final_vision_state = parse_vision_status(raw_text)
        observation = prompt_observation(condition)
        runs.append(
            {
                "run_index": run_index,
                "condition_id": condition.id,
                "command": condition.command,
                "remote_returncode": result.returncode,
                "final_vision_state": final_vision_state,
                "observation": observation,
            }
        )
        if result.returncode != 0:
            exit_code = result.returncode
            break

    raw_log_path.write_text("\n".join(raw_chunks), encoding="utf-8")

    artifact: dict[str, object] = {
        "session": {
            "id": args.session_id,
            "started_at": started_at.isoformat(),
            "candidate_label": args.candidate_label,
            "firmware_label": args.firmware_label,
            "flash_status": args.flash_status,
            "surface_label": args.surface_label,
            "power_label": args.power_label,
            "build_flash_policy": "external_precondition",
            "host": args.host,
            "port": args.port,
            "baud": args.baud,
        },
        "conditions": [make_condition_artifact(condition, args.runs)],
        "runs": runs,
    }
    json_path.write_text(json.dumps(artifact, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    summary_path.write_text(make_summary(artifact, raw_log_path, json_path), encoding="utf-8")

    print(f"Raw log: {raw_log_path}")
    print(f"JSON: {json_path}")
    print(f"Summary: {summary_path}")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
