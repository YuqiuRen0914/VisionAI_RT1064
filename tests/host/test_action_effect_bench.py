import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = REPO_ROOT / "tools" / "bench" / "action_effect_bench.py"


def load_module():
    spec = importlib.util.spec_from_file_location("action_effect_bench", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class FakeRemote:
    def __init__(self):
        self.scripts = []

    def __call__(self, args, script):
        self.scripts.append(script)
        return load_module().RemoteResult(
            returncode=0,
            stdout=(
                "### ACTION_RUN_BEGIN run=1 condition=move_short_up\n"
                ">>> vision sim reset\n"
                "OK vision sim reset\n"
                ">>> stream speed\n"
                "OK stream speed\n"
                ">>> vision sim move up 20\n"
                "OK vision sim move\n"
                ">>> vision\n"
                "OK vision status=1 parser=0 active_seq=1\n"
                ">>> status\n"
                "OK status mode=action_closed_loop armed=0 speed_armed=1 stream=on\n"
                "DATA action cmd=1 dir=1 val=20 elapsed=180 stable=0 blocked=0 vx=90.0 vy=2.0 rot=1.0\n"
                ">>> vision\n"
                "OK vision status=0 parser=0 active_seq=1\n"
                ">>> stream off\n"
                "OK stream off\n"
                ">>> vision\n"
                "OK vision status=0 parser=0 active_seq=1\n"
                ">>> status\n"
                "OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off\n"
                "DATA action cmd=1 dir=1 val=20 elapsed=620 stable=120 blocked=0 vx=0.0 vy=0.0 rot=0.0\n"
                "### ACTION_RUN_END run=1 condition=move_short_up final=IDLE\n"
            ),
            stderr="",
        )


class ActionEffectBenchTests(unittest.TestCase):
    def test_external_thresholds_compute_run_pass_fail_and_missing_blocks_acceptance(self):
        bench = load_module()

        move_condition = bench.CONDITIONS_BY_ID["move_short_up"]
        move_observation = {
            "manual_truth_state": "provided",
            "actual_travel_mm": 205.0,
            "lateral_drift_mm": -8.0,
            "end_heading_error_deg": 2.5,
            "quality": "good",
            "notes": "",
        }
        self.assertEqual(
            bench.evaluate_observation_thresholds(move_condition, move_observation)["pass_fail"],
            "pass",
        )

        rotate_condition = bench.CONDITIONS_BY_ID["rotate_90_cw"]
        rotate_observation = {
            "manual_truth_state": "provided",
            "actual_angle_deg": 95.0,
            "translation_crosstalk_mm": 12.0,
            "end_heading_error_deg": 2.0,
            "quality": "acceptable",
            "notes": "overshot",
        }
        rotate_result = bench.evaluate_observation_thresholds(rotate_condition, rotate_observation)
        self.assertEqual(rotate_result["pass_fail"], "fail")
        self.assertIn("actual_angle_error_deg", rotate_result["failed_metrics"])

        missing_result = bench.evaluate_observation_thresholds(move_condition, {"manual_truth_state": "missing"})
        self.assertEqual(missing_result["pass_fail"], "missing")
        self.assertFalse(missing_result["acceptance_eligible"])

    def test_run_evidence_extracts_status_timeline_and_final_board_snapshot(self):
        bench = load_module()
        raw_text = (
            "### ACTION_RUN_BEGIN run=1 condition=move_short_up\n"
            ">>> vision sim move up 20\n"
            "OK vision sim move\n"
            ">>> vision\n"
            "OK vision status=1 parser=0 active_seq=1\n"
            ">>> status\n"
            "OK status mode=action_closed_loop armed=0 speed_armed=1 stream=on\n"
            "DATA action cmd=1 dir=1 val=20 elapsed=100 stable=0 blocked=0 vx=120.5 vy=0.0 rot=2.0\n"
            ">>> vision\n"
            "OK vision status=1 parser=0 active_seq=1\n"
            ">>> status\n"
            "OK status mode=action_closed_loop armed=0 speed_armed=1 stream=on\n"
            "DATA action cmd=1 dir=1 val=20 elapsed=220 stable=20 blocked=0 vx=80.0 vy=0.0 rot=1.0\n"
            ">>> vision\n"
            "OK vision status=0 parser=0 active_seq=1\n"
            ">>> stream off\n"
            "OK stream off\n"
            ">>> vision\n"
            "OK vision status=0 parser=0 active_seq=1\n"
            ">>> status\n"
            "OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off\n"
            "DATA action cmd=1 dir=1 val=20 elapsed=620 stable=120 blocked=0 vx=0.0 vy=0.0 rot=0.0\n"
            "### ACTION_RUN_END run=1 condition=move_short_up final=IDLE\n"
        )

        evidence = bench.extract_run_evidence(raw_text)

        self.assertEqual([sample["action"]["elapsed"] for sample in evidence["status_timeline"]], [100, 220])
        self.assertEqual(evidence["final_board_snapshot"]["vision"]["status"], "IDLE")
        self.assertEqual(evidence["final_board_snapshot"]["status"]["stream"], "off")
        self.assertEqual(evidence["final_board_snapshot"]["action"]["stable"], 120)

    def test_full_matrix_plan_uses_canonical_round_robin_order(self):
        bench = load_module()

        plan = bench.build_session_plan(scope="full-matrix", repeats=3)

        self.assertEqual(
            [condition.id for condition in plan.conditions],
            [
                "move_short_up",
                "move_short_down",
                "move_short_left",
                "move_short_right",
                "move_long_up",
                "move_long_down",
                "move_long_left",
                "move_long_right",
                "rotate_90_cw",
                "rotate_90_ccw",
                "rotate_180_cw",
                "rotate_180_ccw",
            ],
        )
        self.assertTrue(plan.standard_evidence)
        self.assertEqual(len(plan.runs), 36)
        self.assertEqual([run.condition.id for run in plan.runs[:12]], [c.id for c in plan.conditions])
        self.assertEqual([run.repeat_index for run in plan.runs[:12]], [1] * 12)
        self.assertEqual([run.repeat_index for run in plan.runs[12:24]], [2] * 12)
        self.assertEqual([run.repeat_index for run in plan.runs[24:]], [3] * 12)

    def test_subset_plan_targets_buckets_and_conditions_in_canonical_order(self):
        bench = load_module()

        plan = bench.build_session_plan(
            scope="subset",
            buckets=["rotate-180", "move-short"],
            condition_ids=["rotate_90_ccw", "move_long_left"],
            repeats=3,
        )

        self.assertEqual(
            [condition.id for condition in plan.conditions],
            [
                "move_short_up",
                "move_short_down",
                "move_short_left",
                "move_short_right",
                "move_long_left",
                "rotate_90_ccw",
                "rotate_180_cw",
                "rotate_180_ccw",
            ],
        )
        self.assertEqual(plan.coverage_summary, "subset: move-short, move_long_left, rotate_90_ccw, rotate-180")

    def test_reduced_repeat_plan_is_marked_nonstandard(self):
        bench = load_module()

        plan = bench.build_session_plan(scope="subset", condition_ids=["move_short_up"], repeats=1)

        self.assertFalse(plan.standard_evidence)
        self.assertEqual(plan.evidence_class, "nonstandard")
        self.assertEqual(plan.coverage_summary, "subset: move_short_up; repeats=1 exploratory")

    def test_session_id_allocation_and_candidate_label_generation(self):
        bench = load_module()

        with tempfile.TemporaryDirectory() as tmp:
            sessions = Path(tmp) / "sessions"
            (sessions / "AE01").mkdir(parents=True)
            (sessions / "AE07").mkdir()
            self.assertEqual(bench.allocate_session_id(Path(tmp)), "AE08")

        self.assertEqual(
            bench.generate_candidate_label(
                move=(180.0, 600.5, 3.0, 40.25),
                rotate=(120.0, 400.0, 4.0, 30.0),
                heading=(3.0, 80.0),
            ),
            "m180-600.5-3-40.25_r120-400-4-30_h3-80",
        )

    def test_one_move_condition_session_runs_flow_and_writes_artifacts(self):
        bench = load_module()
        remote = FakeRemote()

        with tempfile.TemporaryDirectory() as tmp:
            argv = [
                "--artifact-root",
                tmp,
                "--session-id",
                "AE99",
                "--firmware-label",
                "fw-test",
                "--flash-status",
                "flashed",
                "--surface-label",
                "mat",
                "--power-label",
                "bench-supply",
                "--action",
                "move",
                "--direction",
                "up",
                "--value",
                "20",
                "--runs",
                "1",
                "--action-move",
                "180 600 3.0 40",
                "--action-rotate",
                "120 400 4.0 30",
                "--action-heading",
                "3.0 80",
            ]
            answers = iter(["record", "205", "3", "-1", "good", "straight enough"])
            with patch("builtins.input", lambda _prompt="": next(answers)), patch("builtins.print"):
                exit_code = bench.main(argv, remote_runner=remote)

            self.assertEqual(exit_code, 0)
            self.assertEqual(len(remote.scripts), 1)
            script = remote.scripts[0]
            expected_order = [
                "action defaults",
                "action show",
                "action move 180 600 3 40",
                "action rotate 120 400 4 30",
                "action heading 3 80",
                "vision sim reset",
                "stream speed",
                "vision sim move up 20",
                "Poll-Vision-Until-Done",
                "stream off",
                "vision",
                "status",
            ]
            positions = []
            search_from = 0
            for item in expected_order:
                found_at = script.index(item, search_from)
                positions.append(found_at)
                search_from = found_at + len(item)
            self.assertEqual(positions, sorted(positions))

            root = Path(tmp)
            raw_log = root / "sessions" / "AE99" / "AE99.raw.log"
            json_path = root / "sessions" / "AE99" / "AE99.json"
            summary_path = root / "sessions" / "AE99" / "AE99.summary.md"
            self.assertTrue(raw_log.exists())
            self.assertTrue(json_path.exists())
            self.assertTrue(summary_path.exists())

            artifact = json.loads(json_path.read_text(encoding="utf-8"))
            self.assertEqual(artifact["session"]["firmware_label"], "fw-test")
            self.assertEqual(artifact["session"]["flash_status"], "flashed")
            self.assertEqual(artifact["session"]["surface_label"], "mat")
            self.assertEqual(artifact["session"]["power_label"], "bench-supply")
            self.assertEqual(artifact["session"]["build_flash_policy"], "external_precondition")
            self.assertEqual(artifact["conditions"][0]["id"], "move_short_up")
            self.assertEqual(artifact["runs"][0]["observation"]["actual_travel_mm"], 205.0)
            self.assertEqual(artifact["runs"][0]["observation"]["pass_fail"], "pass")
            self.assertEqual(artifact["runs"][0]["threshold_evaluation"]["pass_fail"], "pass")
            self.assertEqual(artifact["runs"][0]["status_timeline"][0]["action"]["elapsed"], 180)
            self.assertEqual(
                artifact["runs"][0]["final_board_snapshot"]["status"]["mode"],
                "action_closed_loop",
            )
            summary = summary_path.read_text(encoding="utf-8")
            self.assertIn("Structured Action-Effect Observation", summary)
            self.assertIn("Threshold Evaluation", summary)
            self.assertIn("Action Status Timeline", summary)
            self.assertIn("Final Board Snapshot", summary)

    def test_session_can_record_more_than_one_run(self):
        bench = load_module()
        remote = FakeRemote()

        with tempfile.TemporaryDirectory() as tmp:
            argv = [
                "--artifact-root",
                tmp,
                "--session-id",
                "AE100",
                "--firmware-label",
                "fw-test",
                "--flash-status",
                "flashed",
                "--surface-label",
                "mat",
                "--power-label",
                "bench-supply",
                "--action",
                "rotate",
                "--direction",
                "cw",
                "--value",
                "90",
                "--runs",
                "2",
            ]
            answers = iter(
                [
                    "record",
                    "88",
                    "5",
                    "-2",
                    "acceptable",
                    "slight undershoot",
                    "record",
                    "91",
                    "4",
                    "1",
                    "good",
                    "clean",
                ]
            )
            with patch("builtins.input", lambda _prompt="": next(answers)), patch("builtins.print"):
                exit_code = bench.main(argv, remote_runner=remote)

            self.assertEqual(exit_code, 0)
            self.assertEqual(len(remote.scripts), 2)
            self.assertIn("action defaults", remote.scripts[0])
            self.assertNotIn("action defaults", remote.scripts[1])

            artifact = json.loads(
                (Path(tmp) / "sessions" / "AE100" / "AE100.json").read_text(encoding="utf-8")
            )
            self.assertEqual(artifact["conditions"][0]["id"], "rotate_90_cw")
            self.assertEqual(len(artifact["runs"]), 2)
            self.assertEqual(artifact["runs"][1]["observation"]["actual_angle_deg"], 91.0)

    def test_missing_manual_observation_is_explicit_and_blocks_acceptance(self):
        bench = load_module()
        remote = FakeRemote()

        with tempfile.TemporaryDirectory() as tmp:
            argv = [
                "--artifact-root",
                tmp,
                "--session-id",
                "AE101",
                "--firmware-label",
                "fw-test",
                "--flash-status",
                "flashed",
                "--surface-label",
                "mat",
                "--power-label",
                "bench-supply",
                "--action",
                "move",
                "--direction",
                "up",
                "--value",
                "20",
                "--runs",
                "1",
            ]
            answers = iter(["missing", "operator could not measure"])
            with patch("builtins.input", lambda _prompt="": next(answers)), patch("builtins.print"):
                exit_code = bench.main(argv, remote_runner=remote)

            self.assertEqual(exit_code, 0)
            artifact = json.loads(
                (Path(tmp) / "sessions" / "AE101" / "AE101.json").read_text(encoding="utf-8")
            )
            self.assertFalse(artifact["session"]["acceptance_eligible"])
            self.assertIn("missing_manual_observation", artifact["session"]["acceptance_blockers"])
            self.assertEqual(artifact["runs"][0]["observation"]["manual_truth_state"], "missing")
            self.assertEqual(artifact["runs"][0]["observation"]["pass_fail"], "missing")


if __name__ == "__main__":
    unittest.main()
