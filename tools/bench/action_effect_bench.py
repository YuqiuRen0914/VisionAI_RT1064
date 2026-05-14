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


@dataclass(frozen=True)
class PlannedRun:
    run_index: int
    repeat_index: int
    condition: ActionCondition


@dataclass(frozen=True)
class SessionPlan:
    scope: str
    conditions: tuple[ActionCondition, ...]
    runs: tuple[PlannedRun, ...]
    repeats: int
    standard_evidence: bool
    evidence_class: str
    coverage_summary: str


CANONICAL_CONDITIONS: tuple[ActionCondition, ...] = (
    ActionCondition("move_short_up", "move", "MOVE short", "up", 20, "vision sim move up 20"),
    ActionCondition("move_short_down", "move", "MOVE short", "down", 20, "vision sim move down 20"),
    ActionCondition("move_short_left", "move", "MOVE short", "left", 20, "vision sim move left 20"),
    ActionCondition("move_short_right", "move", "MOVE short", "right", 20, "vision sim move right 20"),
    ActionCondition("move_long_up", "move", "MOVE long", "up", 100, "vision sim move up 100"),
    ActionCondition("move_long_down", "move", "MOVE long", "down", 100, "vision sim move down 100"),
    ActionCondition("move_long_left", "move", "MOVE long", "left", 100, "vision sim move left 100"),
    ActionCondition("move_long_right", "move", "MOVE long", "right", 100, "vision sim move right 100"),
    ActionCondition("rotate_90_cw", "rotate", "ROTATE 90", "cw", 90, "vision sim rotate cw 90"),
    ActionCondition("rotate_90_ccw", "rotate", "ROTATE 90", "ccw", 90, "vision sim rotate ccw 90"),
    ActionCondition("rotate_180_cw", "rotate", "ROTATE 180", "cw", 180, "vision sim rotate cw 180"),
    ActionCondition("rotate_180_ccw", "rotate", "ROTATE 180", "ccw", 180, "vision sim rotate ccw 180"),
)

CONDITIONS_BY_ID = {condition.id: condition for condition in CANONICAL_CONDITIONS}
BUCKET_ALIASES = {
    "move-short": "MOVE short",
    "move_short": "MOVE short",
    "MOVE short": "MOVE short",
    "move-long": "MOVE long",
    "move_long": "MOVE long",
    "MOVE long": "MOVE long",
    "rotate-90": "ROTATE 90",
    "rotate_90": "ROTATE 90",
    "ROTATE 90": "ROTATE 90",
    "rotate-180": "ROTATE 180",
    "rotate_180": "ROTATE 180",
    "ROTATE 180": "ROTATE 180",
}


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


def normalize_bucket(value: str) -> str:
    try:
        return BUCKET_ALIASES[value]
    except KeyError as exc:
        choices = ", ".join(sorted({key for key in BUCKET_ALIASES if key.islower()}))
        raise argparse.ArgumentTypeError(f"unknown bucket {value!r}; expected one of: {choices}") from exc


def build_session_plan(
    *,
    scope: str,
    buckets: list[str] | None = None,
    condition_ids: list[str] | None = None,
    repeats: int = 3,
) -> SessionPlan:
    if repeats <= 0:
        raise argparse.ArgumentTypeError("repeats must be > 0")

    buckets = buckets or []
    condition_ids = condition_ids or []

    if scope == "full-matrix":
        selected_conditions = CANONICAL_CONDITIONS
        coverage_summary = "full-matrix"
    elif scope == "subset":
        bucket_names = {normalize_bucket(bucket) for bucket in buckets}
        selected_ids = set(condition_ids)
        unknown_ids = sorted(selected_ids - CONDITIONS_BY_ID.keys())
        if unknown_ids:
            raise argparse.ArgumentTypeError(f"unknown condition id: {', '.join(unknown_ids)}")
        if not bucket_names and not selected_ids:
            raise argparse.ArgumentTypeError("subset scope requires --bucket or --condition")
        selected_conditions = tuple(
            condition
            for condition in CANONICAL_CONDITIONS
            if condition.bucket in bucket_names or condition.id in selected_ids
        )
        canonical_index = {condition.id: index for index, condition in enumerate(CANONICAL_CONDITIONS)}
        coverage_entries: list[tuple[int, str]] = []
        for bucket in buckets:
            bucket_name = normalize_bucket(bucket)
            first_bucket_index = min(
                index
                for index, condition in enumerate(CANONICAL_CONDITIONS)
                if condition.bucket == bucket_name
            )
            coverage_entries.append((first_bucket_index, bucket))
        for condition_id in condition_ids:
            coverage_entries.append((canonical_index[condition_id], condition_id))
        coverage_parts = [label for _index, label in sorted(coverage_entries)]
        coverage_summary = "subset: " + ", ".join(coverage_parts)
    else:
        raise argparse.ArgumentTypeError("scope must be full-matrix or subset")

    runs = tuple(
        PlannedRun(
            run_index=((repeat_index - 1) * len(selected_conditions)) + condition_offset + 1,
            repeat_index=repeat_index,
            condition=condition,
        )
        for repeat_index in range(1, repeats + 1)
        for condition_offset, condition in enumerate(selected_conditions)
    )
    standard_evidence = repeats == 3
    evidence_class = "standard" if standard_evidence else "nonstandard"
    if not standard_evidence:
        coverage_summary = f"{coverage_summary}; repeats={repeats} exploratory"

    return SessionPlan(
        scope=scope,
        conditions=selected_conditions,
        runs=runs,
        repeats=repeats,
        standard_evidence=standard_evidence,
        evidence_class=evidence_class,
        coverage_summary=coverage_summary,
    )


def allocate_session_id(artifact_root: Path) -> str:
    sessions_dir = artifact_root / "sessions"
    highest = 0
    if sessions_dir.exists():
        for path in sessions_dir.iterdir():
            match = re.fullmatch(r"AE(\d+)", path.name)
            if match:
                highest = max(highest, int(match.group(1)))
    return f"AE{highest + 1:02d}"


def generate_candidate_label(
    *,
    move: tuple[float, ...] | None,
    rotate: tuple[float, ...] | None,
    heading: tuple[float, ...] | None,
) -> str:
    def group(prefix: str, values: tuple[float, ...] | None) -> str:
        if values is None:
            return f"{prefix}default"
        return prefix + "-".join(fmt_float(value) for value in values)

    return "_".join([group("m", move), group("r", rotate), group("h", heading)])


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


def make_single_condition_plan(condition: ActionCondition, repeats: int) -> SessionPlan:
    standard_evidence = repeats == 3
    evidence_class = "standard" if standard_evidence else "nonstandard"
    coverage_summary = condition.id
    if not standard_evidence:
        coverage_summary = f"{coverage_summary}; repeats={repeats} exploratory"
    return SessionPlan(
        scope="subset",
        conditions=(condition,),
        runs=tuple(
            PlannedRun(run_index=run_index, repeat_index=run_index, condition=condition)
            for run_index in range(1, repeats + 1)
        ),
        repeats=repeats,
        standard_evidence=standard_evidence,
        evidence_class=evidence_class,
        coverage_summary=coverage_summary,
    )


def make_summary(artifact: dict[str, object], raw_log_path: Path, json_path: Path) -> str:
    session = artifact["session"]
    conditions = artifact["conditions"]
    runs = artifact["runs"]
    lines = [
        "# Action-Effect Bench Summary",
        "",
        f"- Session: `{session['id']}`",
        f"- Candidate: `{session['candidate_label']}`",
        f"- Scope: `{session['coverage_summary']}`",
        f"- Evidence: `{session['evidence_class']}`",
        f"- Firmware: `{session['firmware_label']}`",
        f"- Flash status: `{session['flash_status']}`",
        f"- Surface: `{session['surface_label']}`",
        f"- Power: `{session['power_label']}`",
        f"- Build/flash policy: `{session['build_flash_policy']}`",
        f"- Raw log: `{raw_log_path}`",
        f"- JSON: `{json_path}`",
        "",
        "## Conditions",
        "",
    ]
    for condition in conditions:
        lines.append(
            f"- `{condition['id']}` {condition['bucket']} {condition['direction']} "
            f"value={condition['value']} repeats={condition['selected_runs']}"
        )
    lines.extend(
        [
            "",
            "## Structured Action-Effect Observation",
            "",
            "| Run | Repeat | Condition | Final vision | Pass/fail | Quality | Observation | Notes |",
            "| --- | ------ | --------- | ------------ | --------- | ------- | ----------- | ----- |",
        ]
    )

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
                    str(run["repeat_index"]),
                    f"`{run['condition_id']}`",
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
    parser.add_argument("--session-id")
    parser.add_argument("--candidate-label", default="")
    parser.add_argument("--firmware-label", required=True)
    parser.add_argument("--flash-status", required=True)
    parser.add_argument("--surface-label", required=True)
    parser.add_argument("--power-label", required=True)
    parser.add_argument("--scope", choices=["full-matrix", "subset"])
    parser.add_argument("--bucket", action="append", default=[])
    parser.add_argument("--condition", dest="condition_ids", action="append", default=[])
    parser.add_argument("--action", choices=["move", "rotate"])
    parser.add_argument("--direction")
    parser.add_argument("--value", type=int)
    parser.add_argument("--runs", type=int)
    parser.add_argument("--repeats", type=int)
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

    if (args.action is None) != (args.direction is None) or (args.action is None) != (args.value is None):
        parser.error("--action, --direction, and --value must be provided together")

    repeats = args.repeats if args.repeats is not None else args.runs
    if repeats is None:
        repeats = 1 if args.action and args.scope is None else 3
    if repeats <= 0:
        parser.error("--repeats/--runs must be > 0")
    args.runs = repeats
    args.repeats = repeats

    if args.run_timeout_s <= 0.0:
        parser.error("--run-timeout-s must be > 0")
    if args.poll_interval_ms <= 0:
        parser.error("--poll-interval-ms must be > 0")

    if args.action:
        try:
            legacy_condition = build_condition(args.action, args.direction, args.value)
        except argparse.ArgumentTypeError as exc:
            parser.error(str(exc))
        if args.scope is None:
            args.plan = make_single_condition_plan(legacy_condition, args.repeats)
        else:
            args.condition_ids.append(legacy_condition.id)

    if not hasattr(args, "plan"):
        scope = args.scope or "full-matrix"
        try:
            args.plan = build_session_plan(
                scope=scope,
                buckets=args.bucket,
                condition_ids=args.condition_ids,
                repeats=args.repeats,
            )
        except argparse.ArgumentTypeError as exc:
            parser.error(str(exc))

    if not args.session_id:
        args.session_id = allocate_session_id(args.artifact_root)

    if not args.candidate_label:
        args.candidate_label = generate_candidate_label(
            move=args.action_move,
            rotate=args.action_rotate,
            heading=args.action_heading,
        )

    return args


def main(
    argv: list[str],
    remote_runner: Callable[[argparse.Namespace, str], RemoteResult] | None = None,
) -> int:
    args = parse_args(argv)
    remote_runner = remote_runner or run_remote
    plan: SessionPlan = args.plan
    setup_commands = run_setup_commands(args)

    if args.plan_only:
        print("# Action-effect bench plan")
        print(f"session={args.session_id} scope={plan.coverage_summary} runs={len(plan.runs)}")
        print(f"evidence={plan.evidence_class}")
        for command in setup_commands:
            print(command)
        for planned_run in plan.runs:
            print(
                f"run {planned_run.run_index} repeat={planned_run.repeat_index} "
                f"condition={planned_run.condition.id}: vision sim reset"
            )
            print(f"run {planned_run.run_index}: stream speed")
            print(f"run {planned_run.run_index}: {planned_run.condition.command}")
            print(f"run {planned_run.run_index}: poll vision until not BUSY")
            print(f"run {planned_run.run_index}: stream off")
            print(f"run {planned_run.run_index}: vision")
            print(f"run {planned_run.run_index}: status")
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

    for planned_run in plan.runs:
        condition = planned_run.condition
        per_run_setup = setup_commands if planned_run.run_index == 1 else []
        script = build_remote_script(args, condition, planned_run.run_index, per_run_setup)
        result = remote_runner(args, script)
        raw_text = result.stdout
        if result.stderr:
            raw_text += "\n### STDERR\n" + result.stderr
        raw_chunks.append(raw_text)

        final_vision_state = parse_vision_status(raw_text)
        observation = prompt_observation(condition)
        runs.append(
            {
                "run_index": planned_run.run_index,
                "repeat_index": planned_run.repeat_index,
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
            "scope": plan.scope,
            "coverage_summary": plan.coverage_summary,
            "standard_evidence": plan.standard_evidence,
            "evidence_class": plan.evidence_class,
            "repeats": plan.repeats,
            "host": args.host,
            "port": args.port,
            "baud": args.baud,
        },
        "conditions": [make_condition_artifact(condition, plan.repeats) for condition in plan.conditions],
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
