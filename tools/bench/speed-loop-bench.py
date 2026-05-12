#!/usr/bin/env python3
"""Run repeatable speed-loop bench tests through the Windows COM port."""

from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LOG_DIR = REPO_ROOT / ".scratch" / "closed-loop-control" / "serial-logs"
DEFAULT_HOST = "Windows@10.211.55.3"
DEFAULT_PORT = "COM3"
DEFAULT_BAUD = 115200
DEFAULT_SSH_KEY = Path.home() / ".ssh" / "keil_windows_ed25519"
MAC_DOCUMENTS_PREFIX = Path("/Volumes/[C] Windows 11/Users/jjp/Documents")
WINDOWS_DOCUMENTS_PREFIX = r"C:\Users\jjp\Documents"

STAGES = {
    "initial": (30.0, 200.0),
    "normal": (50.0, 400.0),
    "extended": (70.0, 600.0),
}

SPEED_LINE_RE = re.compile(r"^DATA speed\s+(?P<body>.*)$")
STEP_BEGIN_RE = re.compile(r"^### STEP_BEGIN\s+(?P<body>.*)$")
STEP_END_RE = re.compile(r"^### STEP_END\s+(?P<body>.*)$")


@dataclass(frozen=True)
class Step:
    name: str
    kind: str
    command: str
    duration_s: float
    targets: tuple[float, float, float, float]


@dataclass
class ParsedStep:
    step: Step
    samples: list[dict[str, float]]


def ps_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def parse_csv_floats(text: str) -> list[float]:
    values: list[float] = []
    for item in text.split(","):
        item = item.strip()
        if not item:
            continue
        values.append(float(item))
    if not values:
        raise argparse.ArgumentTypeError("expected at least one numeric value")
    return values


def fmt_float(value: float) -> str:
    if abs(value - round(value)) < 0.0001:
        return str(int(round(value)))
    return f"{value:.3f}".rstrip("0").rstrip(".")


def build_steps(args: argparse.Namespace) -> list[Step]:
    targets = parse_csv_floats(args.targets)
    steps: list[Step] = []

    def duration_for_target(target: float, all_wheel: bool) -> float:
        if abs(target) <= 50.0 and target != 0.0:
            return args.duration_low
        return args.duration_all if all_wheel else args.duration_single

    def stop_step(name: str, command: str) -> Step:
        return Step(
            name=f"{name}_stop",
            kind="stop",
            command=command,
            duration_s=args.duration_stop,
            targets=(0.0, 0.0, 0.0, 0.0),
        )

    if args.profile in ("quick", "single", "default"):
        wheels = [args.wheel] if args.wheel else ([1] if args.profile == "quick" else [1, 2, 3, 4])
        for wheel in wheels:
            for target in targets:
                target_tuple = [0.0, 0.0, 0.0, 0.0]
                target_tuple[wheel - 1] = target
                name = f"w{wheel}_{fmt_float(target)}"
                steps.append(
                    Step(
                        name=name,
                        kind="single",
                        command=f"speed wheel {wheel} {fmt_float(target)}",
                        duration_s=duration_for_target(target, all_wheel=False),
                        targets=tuple(target_tuple),  # type: ignore[arg-type]
                    )
                )
                steps.append(stop_step(name, f"speed wheel {wheel} 0"))

    if args.profile in ("all", "default"):
        for target in targets:
            name = f"all_{fmt_float(target)}"
            steps.append(
                Step(
                    name=name,
                    kind="all",
                    command=(
                        f"speed all {fmt_float(target)} {fmt_float(target)} "
                        f"{fmt_float(target)} {fmt_float(target)}"
                    ),
                    duration_s=duration_for_target(target, all_wheel=True),
                    targets=(target, target, target, target),
                )
            )
            steps.append(stop_step(name, "speed all 0 0 0 0"))

        if args.include_mixed:
            mixed_targets = (
                (100.0, -100.0, 100.0, -100.0),
                (-100.0, 100.0, -100.0, 100.0),
            )
            for index, target_tuple in enumerate(mixed_targets, start=1):
                name = f"mixed_{index}"
                steps.append(
                    Step(
                        name=name,
                        kind="mixed",
                        command="speed all "
                        + " ".join(fmt_float(value) for value in target_tuple),
                        duration_s=args.duration_mixed,
                        targets=target_tuple,
                    )
                )
                steps.append(stop_step(name, "speed all 0 0 0 0"))

    return steps


def build_remote_script(args: argparse.Namespace, steps: list[Step]) -> str:
    setup_commands = [
        ("stream off", 0.25),
        ("speed stop", 0.30),
        ("status", 0.30),
        ("speed arm", 0.35),
        (f"speed limit {fmt_float(args.duty_limit)} {fmt_float(args.max_speed)}", 0.25),
        (f"speed pid {fmt_float(args.kp)} {fmt_float(args.ki)} {fmt_float(args.kd)}", 0.25),
    ]

    if args.static_duty is not None:
        setup_commands.append(
            (
                f"speed static {fmt_float(args.static_duty)} {fmt_float(args.static_threshold)}",
                0.25,
            )
        )

    if args.feedforward is not None:
        setup_commands.append((f"speed ff {fmt_float(args.feedforward)}", 0.25))

    if args.speed_filter is not None:
        setup_commands.append((f"speed filter {fmt_float(args.speed_filter)}", 0.25))

    setup_commands.append(("stream speed", 0.50))

    step_blocks = []
    for step in steps:
        target_text = ",".join(fmt_float(value) for value in step.targets)
        step_blocks.append(
            "Run-Step "
            f"{ps_quote(step.name)} "
            f"{ps_quote(step.kind)} "
            f"{ps_quote(target_text)} "
            f"{ps_quote(step.command)} "
            f"{step.duration_s:.3f}"
        )

    setup_blocks = [
        f"Send-Line {ps_quote(command)} {wait_s:.3f}" for command, wait_s in setup_commands
    ]

    cleanup_blocks = [
        "Send-Line 'speed all 0 0 0 0' 0.500",
        "Send-Line 'speed stop' 0.500",
        "Send-Line 'stream off' 0.300",
        "Send-Line 'status' 0.500",
    ]

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
}}

function Send-Line([string]$cmd, [double]$waitSeconds) {{
  Emit ">>> $cmd"
  $sp.WriteLine($cmd)
  Start-Sleep -Milliseconds 40
  Read-For $waitSeconds
}}

function Run-Step([string]$name, [string]$kind, [string]$targets, [string]$cmd, [double]$durationSeconds) {{
  Emit "### STEP_BEGIN name=$name kind=$kind duration_s=$durationSeconds targets=$targets"
  Send-Line $cmd $durationSeconds
  Emit "### STEP_END name=$name"
}}

$sp.Open()
try {{
  Start-Sleep -Milliseconds 200
  $sp.DiscardInBuffer()
  Emit "### BENCH_BEGIN port=$portName baud=$baud"
  {"; ".join(setup_blocks)}
  {"; ".join(step_blocks)}
  {"; ".join(cleanup_blocks)}
  Emit "### BENCH_END"
}}
finally {{
  if ($sp.IsOpen) {{
    try {{
      $sp.WriteLine('speed all 0 0 0 0')
      Start-Sleep -Milliseconds 50
      $sp.WriteLine('speed stop')
      Start-Sleep -Milliseconds 50
      $sp.WriteLine('stream off')
      Start-Sleep -Milliseconds 50
      $tail = $sp.ReadExisting()
      if ($tail.Length -gt 0) {{ EmitRaw ($tail.Replace("`r", "")) }}
    }} catch {{}}
    $sp.Close()
  }}
}}
""".lstrip()


def parse_kv_body(body: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in body.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        result[key] = value.rstrip(",")
    return result


def parse_speed_line(line: str) -> dict[str, float] | None:
    match = SPEED_LINE_RE.match(line.strip())
    if not match:
        return None

    parsed: dict[str, float] = {}
    for key, value in parse_kv_body(match.group("body")).items():
        try:
            parsed[key] = float(value)
        except ValueError:
            continue
    return parsed


def parse_targets(text: str) -> tuple[float, float, float, float]:
    values = [float(item) for item in text.split(",")]
    if len(values) != 4:
        return (0.0, 0.0, 0.0, 0.0)
    return (values[0], values[1], values[2], values[3])


def parse_log(text: str, planned_steps: list[Step]) -> list[ParsedStep]:
    text = text.replace("### STEP_BEGIN", "\n### STEP_BEGIN")
    text = text.replace("### STEP_END", "\n### STEP_END")
    text = text.replace("DATA speed", "\nDATA speed")
    planned_by_name = {step.name: step for step in planned_steps}
    parsed_steps: list[ParsedStep] = []
    current: ParsedStep | None = None

    for raw_line in text.splitlines():
        line = raw_line.strip()
        begin = STEP_BEGIN_RE.match(line)
        if begin:
            meta = parse_kv_body(begin.group("body"))
            name = meta.get("name", "unknown")
            step = planned_by_name.get(
                name,
                Step(
                    name=name,
                    kind=meta.get("kind", "unknown"),
                    command="",
                    duration_s=float(meta.get("duration_s", "0") or 0.0),
                    targets=parse_targets(meta.get("targets", "0,0,0,0")),
                ),
            )
            current = ParsedStep(step=step, samples=[])
            parsed_steps.append(current)
            continue

        if STEP_END_RE.match(line):
            current = None
            continue

        sample = parse_speed_line(line)
        if sample is not None and current is not None:
            current.samples.append(sample)

    return parsed_steps


def mean(values: list[float]) -> float:
    return statistics.fmean(values) if values else 0.0


def trimmed_samples(samples: list[dict[str, float]]) -> list[dict[str, float]]:
    if len(samples) < 5:
        return samples
    return samples[len(samples) // 5 :]


def summarize_step(parsed: ParsedStep, duty_limit: float) -> dict[str, str]:
    samples = parsed.samples
    response_samples = trimmed_samples(samples)
    dt_values = [sample.get("dt_ms", 0.0) for sample in samples if sample.get("dt_ms", 0.0) > 0.0]
    dt_summary = "none"
    if dt_values:
        dt_summary = f"{min(dt_values):.0f}/{mean(dt_values):.1f}/{max(dt_values):.0f}"

    active_wheels = [
        index + 1 for index, target in enumerate(parsed.step.targets) if abs(target) > 0.001
    ]
    if not active_wheels:
        active_wheels = [1, 2, 3, 4]

    measured_parts: list[str] = []
    duty_parts: list[str] = []
    saturation_flags: list[str] = []
    conclusion_flags: list[str] = []

    for wheel in active_wheels:
        target = parsed.step.targets[wheel - 1]
        m_key = f"m{wheel}"
        r_key = f"r{wheel}"
        d_key = f"d{wheel}"
        measured = [sample.get(m_key, 0.0) for sample in response_samples]
        raw_measured = [sample[r_key] for sample in response_samples if r_key in sample]
        duties = [sample.get(d_key, 0.0) for sample in response_samples]
        if not measured:
            measured_parts.append(f"w{wheel}: no samples")
            duty_parts.append(f"w{wheel}: no samples")
            conclusion_flags.append(f"w{wheel}:NO_DATA")
            continue

        measured_text = (
            f"w{wheel}: filt_mean={mean(measured):.1f} "
            f"filt_min={min(measured):.1f} filt_max={max(measured):.1f}"
        )
        if raw_measured:
            measured_text += (
                f" raw_mean={mean(raw_measured):.1f} "
                f"raw_min={min(raw_measured):.1f} raw_max={max(raw_measured):.1f}"
            )
        measured_parts.append(measured_text)
        duty_parts.append(
            f"w{wheel}: mean={mean(duties):.1f} min={min(duties):.1f} max={max(duties):.1f}"
        )

        saturated = [
            duty for duty in duties if abs(duty) >= (abs(duty_limit) * 0.95)
        ]
        sat_ratio = (len(saturated) / len(duties)) if duties else 0.0
        saturation_flags.append(f"w{wheel}:{sat_ratio:.0%}")

        if abs(target) > 0.001:
            same_direction = [value for value in measured if (value * target) > 0.0]
            same_ratio = len(same_direction) / len(measured)
            if same_ratio < 0.60:
                conclusion_flags.append(f"w{wheel}:SIGN_CHECK")
            if abs(mean(measured)) < (abs(target) * 0.25):
                conclusion_flags.append(f"w{wheel}:WEAK_RESPONSE")
            if sat_ratio > 0.50:
                conclusion_flags.append(f"w{wheel}:SATURATED")
        else:
            if max(abs(value) for value in measured) > 30.0:
                conclusion_flags.append(f"w{wheel}:STOP_DRIFT")
            if max(abs(value) for value in duties) > 0.1:
                conclusion_flags.append(f"w{wheel}:STOP_DUTY")

    if not samples:
        conclusion = "NO_DATA"
    elif conclusion_flags:
        conclusion = "CHECK " + ", ".join(conclusion_flags)
    else:
        conclusion = "PASS"

    return {
        "samples": str(len(samples)),
        "dt": dt_summary,
        "measured": "<br>".join(measured_parts),
        "duty": "<br>".join(duty_parts),
        "saturation": ", ".join(saturation_flags),
        "conclusion": conclusion,
    }


def make_summary(
    args: argparse.Namespace,
    raw_log_path: Path,
    parsed_steps: list[ParsedStep],
    started_at: dt.datetime,
) -> str:
    lines = [
        "# Speed-loop Bench Summary",
        "",
        f"- Date: {started_at.strftime('%Y-%m-%d %H:%M:%S %Z')}",
        f"- Host: `{args.host}`",
        f"- Port: `{args.port}` at `{args.baud}`",
        f"- Profile: `{args.profile}`",
        f"- Stage: `{args.stage}`",
        f"- PID: `kp={args.kp} ki={args.ki} kd={args.kd}`",
        f"- Limits: `duty={args.duty_limit}% max_speed={args.max_speed} mm/s`",
        f"- Static: `{args.static_duty if args.static_duty is not None else 'unchanged'} duty`, threshold `{args.static_threshold} mm/s`",
        f"- Feedforward: `{args.feedforward if args.feedforward is not None else 'unchanged'}`",
        f"- Speed filter: `{args.speed_filter if args.speed_filter is not None else 'unchanged'} ms`",
        f"- Raw log: `{raw_log_path}`",
        "",
        "| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s filtered/raw | Duty % | Saturation | Conclusion |",
        "| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |",
    ]

    for parsed in parsed_steps:
        summary = summarize_step(parsed, args.duty_limit)
        target_text = ",".join(fmt_float(value) for value in parsed.step.targets)
        lines.append(
            "| "
            + " | ".join(
                [
                    f"`{parsed.step.name}`",
                    f"`{parsed.step.command}`",
                    f"`{target_text}`",
                    summary["samples"],
                    summary["dt"],
                    summary["measured"],
                    summary["duty"],
                    summary["saturation"],
                    summary["conclusion"],
                ]
            )
            + " |"
        )

    lines.extend(
        [
            "",
            "Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.",
            "",
        ]
    )
    return "\n".join(lines)


def mac_path_to_windows(path: Path) -> str:
    resolved = path.resolve()
    try:
        relative = resolved.relative_to(MAC_DOCUMENTS_PREFIX)
    except ValueError as exc:
        raise ValueError(f"path is not under Windows shared Documents: {resolved}") from exc
    return WINDOWS_DOCUMENTS_PREFIX + "\\" + "\\".join(relative.parts)


def windows_double_quote(value: str) -> str:
    return '"' + value.replace('"', '\\"') + '"'


def run_remote(args: argparse.Namespace, script: str) -> subprocess.CompletedProcess[str]:
    script_path = args.log_dir / "_speed-loop-bench-current.ps1"
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
        return subprocess.run(
            command,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            check=False,
        )
    finally:
        try:
            script_path.unlink()
        except OSError:
            pass


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=os.environ.get("KEIL_SSH_HOST", DEFAULT_HOST))
    parser.add_argument("--ssh-key", type=Path, default=DEFAULT_SSH_KEY)
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--log-dir", type=Path, default=DEFAULT_LOG_DIR)
    parser.add_argument("--profile", choices=["quick", "single", "all", "default"], default="quick")
    parser.add_argument("--wheel", type=int, choices=[1, 2, 3, 4])
    parser.add_argument("--targets", default="100,-100,200,-200")
    parser.add_argument("--include-mixed", action="store_true")
    parser.add_argument("--stage", choices=sorted(STAGES), default="initial")
    parser.add_argument("--duty-limit", type=float)
    parser.add_argument("--max-speed", type=float)
    parser.add_argument("--kp", type=float, default=0.05)
    parser.add_argument("--ki", type=float, default=0.02)
    parser.add_argument("--kd", type=float, default=0.0)
    parser.add_argument("--static-duty", type=float)
    parser.add_argument("--static-threshold", type=float, default=60.0)
    parser.add_argument("--feedforward", type=float)
    parser.add_argument("--speed-filter", type=float)
    parser.add_argument("--duration-single", type=float, default=4.0)
    parser.add_argument("--duration-all", type=float, default=3.0)
    parser.add_argument("--duration-mixed", type=float, default=3.0)
    parser.add_argument("--duration-low", type=float, default=5.0)
    parser.add_argument("--duration-stop", type=float, default=1.5)
    parser.add_argument("--label", default="")
    parser.add_argument("--plan-only", action="store_true")
    args = parser.parse_args(argv)

    stage_duty, stage_speed = STAGES[args.stage]
    if args.duty_limit is None:
        args.duty_limit = stage_duty
    if args.max_speed is None:
        args.max_speed = stage_speed

    if args.duty_limit <= 0.0 or args.duty_limit > stage_duty:
        parser.error(f"--duty-limit must be >0 and <= {stage_duty} for stage {args.stage}")
    if args.max_speed <= 0.0 or args.max_speed > stage_speed:
        parser.error(f"--max-speed must be >0 and <= {stage_speed} for stage {args.stage}")
    if args.static_duty is not None and args.static_duty < 0.0:
        parser.error("--static-duty must be >= 0")
    if args.feedforward is not None and args.feedforward < 0.0:
        parser.error("--feedforward must be >= 0")

    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    steps = build_steps(args)
    script = build_remote_script(args, steps)

    if args.plan_only:
        print("# Speed-loop bench plan")
        print(f"host={args.host} port={args.port} baud={args.baud}")
        print(f"stage={args.stage} duty_limit={args.duty_limit} max_speed={args.max_speed}")
        for step in steps:
            print(f"{step.name}: {step.command} for {step.duration_s:.1f}s targets={step.targets}")
        return 0

    started_at = dt.datetime.now().astimezone()
    timestamp = started_at.strftime("%Y%m%d-%H%M%S")
    label = f"-{args.label}" if args.label else ""
    args.log_dir.mkdir(parents=True, exist_ok=True)
    raw_log_path = args.log_dir / f"speed-bench{label}-{timestamp}.log"
    summary_path = args.log_dir / f"speed-bench{label}-{timestamp}.summary.md"

    result = run_remote(args, script)
    raw_text = result.stdout
    if result.stderr:
        raw_text += "\n### STDERR\n" + result.stderr
    raw_log_path.write_text(raw_text, encoding="utf-8")

    parsed_steps = parse_log(raw_text, steps)
    summary = make_summary(args, raw_log_path, parsed_steps, started_at)
    summary_path.write_text(summary, encoding="utf-8")

    print(f"Raw log: {raw_log_path}")
    print(f"Summary: {summary_path}")
    if result.returncode != 0:
        print(f"Remote bench failed with exit code {result.returncode}", file=sys.stderr)
        return result.returncode

    print(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
