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
            self.assertEqual(artifact["session"]["primary_status"], "exploratory")
            self.assertEqual(artifact["session"]["quality_flag"], "none")
            self.assertEqual(artifact["conditions"][0]["id"], "move_short_up")
            self.assertEqual(artifact["conditions"][0]["run_count"], 1)
            self.assertEqual(artifact["conditions"][0]["pass_count"], 1)
            self.assertEqual(artifact["conditions"][0]["quality_counts"]["good"], 1)
            self.assertEqual(
                artifact["conditions"][0]["manual_aggregates"]["actual_travel_mm"],
                {"median": 205.0, "min": 205.0, "max": 205.0},
            )
            self.assertEqual(artifact["runs"][0]["observation"]["actual_travel_mm"], 205.0)
            self.assertEqual(artifact["runs"][0]["observation"]["pass_fail"], "pass")
            self.assertEqual(artifact["runs"][0]["threshold_evaluation"]["pass_fail"], "pass")
            self.assertEqual(artifact["runs"][0]["status_timeline"][0]["action"]["elapsed"], 180)
            self.assertEqual(
                artifact["runs"][0]["final_board_snapshot"]["status"]["mode"],
                "action_closed_loop",
            )
            summary = summary_path.read_text(encoding="utf-8")
            self.assertIn("Primary status: `exploratory`", summary)
            self.assertIn("Quality flag: `none`", summary)
            self.assertIn("Condition Aggregates", summary)
            self.assertIn("### `move_short_up`", summary)
            self.assertIn("Pass: `1/1`", summary)
            self.assertIn("Per-Run Details", summary)
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

    def test_condition_aggregates_use_pass_counts_quality_counts_and_median_spread(self):
        bench = load_module()
        condition = bench.CONDITIONS_BY_ID["move_short_up"]
        runs = [
            {
                "condition_id": condition.id,
                "observation": {
                    "manual_truth_state": "provided",
                    "actual_travel_mm": 198.0,
                    "lateral_drift_mm": -3.0,
                    "end_heading_error_deg": 1.0,
                    "quality": "good",
                },
                "threshold_evaluation": {"pass_fail": "pass"},
            },
            {
                "condition_id": condition.id,
                "observation": {
                    "manual_truth_state": "provided",
                    "actual_travel_mm": 205.0,
                    "lateral_drift_mm": 4.0,
                    "end_heading_error_deg": -1.5,
                    "quality": "acceptable",
                },
                "threshold_evaluation": {"pass_fail": "pass"},
            },
            {
                "condition_id": condition.id,
                "observation": {
                    "manual_truth_state": "provided",
                    "actual_travel_mm": 220.0,
                    "lateral_drift_mm": 12.0,
                    "end_heading_error_deg": 4.0,
                    "quality": "bad",
                },
                "threshold_evaluation": {"pass_fail": "fail"},
            },
        ]

        aggregate = bench.make_condition_artifact(condition, 3, runs)

        self.assertEqual(aggregate["run_count"], 3)
        self.assertEqual(aggregate["pass_count"], 2)
        self.assertEqual(aggregate["fail_count"], 1)
        self.assertEqual(aggregate["missing_count"], 0)
        self.assertEqual(aggregate["quality_counts"], {"good": 1, "acceptable": 1, "bad": 1})
        self.assertEqual(
            aggregate["manual_aggregates"]["actual_travel_mm"],
            {"median": 205.0, "min": 198.0, "max": 220.0},
        )
        self.assertEqual(
            aggregate["manual_aggregates"]["lateral_drift_mm"],
            {"median": 4.0, "min": -3.0, "max": 12.0},
        )

    def test_session_primary_status_rules_are_derived_from_this_session_only(self):
        bench = load_module()

        def make_run(planned_run, pass_fail="pass", quality="good", remote_returncode=0):
            return {
                "run_index": planned_run.run_index,
                "repeat_index": planned_run.repeat_index,
                "condition_id": planned_run.condition.id,
                "remote_returncode": remote_returncode,
                "observation": {
                    "manual_truth_state": "provided" if pass_fail != "missing" else "missing",
                    "actual_travel_mm": 200.0,
                    "lateral_drift_mm": 0.0,
                    "end_heading_error_deg": 0.0,
                    "quality": quality,
                    "notes": "",
                },
                "threshold_evaluation": {"pass_fail": pass_fail, "failed_metrics": []},
            }

        full_plan = bench.build_session_plan(scope="full-matrix", repeats=3)
        self.assertEqual(
            bench.derive_session_judgment(full_plan, [make_run(run) for run in full_plan.runs])[
                "primary_status"
            ],
            "accepted",
        )

        subset_plan = bench.build_session_plan(
            scope="subset", condition_ids=["move_short_up"], repeats=3
        )
        self.assertEqual(
            bench.derive_session_judgment(
                subset_plan, [make_run(run) for run in subset_plan.runs]
            )["primary_status"],
            "scoped-pass",
        )

        rejected_runs = [make_run(run) for run in subset_plan.runs]
        rejected_runs[1]["threshold_evaluation"]["pass_fail"] = "fail"
        self.assertEqual(
            bench.derive_session_judgment(subset_plan, rejected_runs)["primary_status"],
            "rejected",
        )

        exploratory_plan = bench.build_session_plan(
            scope="subset", condition_ids=["move_short_up"], repeats=1
        )
        self.assertEqual(
            bench.derive_session_judgment(
                exploratory_plan, [make_run(exploratory_plan.runs[0], pass_fail="fail")]
            )["primary_status"],
            "exploratory",
        )

        partial_runs = [make_run(run) for run in subset_plan.runs]
        partial_runs[2] = make_run(subset_plan.runs[2], pass_fail="missing")
        self.assertEqual(
            bench.derive_session_judgment(subset_plan, partial_runs)["primary_status"],
            "partial",
        )

        aborted_runs = [make_run(subset_plan.runs[0], remote_returncode=255)]
        self.assertEqual(
            bench.derive_session_judgment(subset_plan, aborted_runs)["primary_status"],
            "aborted",
        )

    def test_quality_flag_is_separate_and_tracks_grades_notes_and_unstable_spread(self):
        bench = load_module()
        plan = bench.build_session_plan(scope="subset", condition_ids=["move_short_up"], repeats=3)

        def make_run(planned_run, metrics, quality="good", notes=""):
            return {
                "run_index": planned_run.run_index,
                "repeat_index": planned_run.repeat_index,
                "condition_id": planned_run.condition.id,
                "remote_returncode": 0,
                "observation": {
                    "manual_truth_state": "provided",
                    "actual_travel_mm": 200.0 + metrics["actual_travel_error_mm"],
                    "lateral_drift_mm": metrics["lateral_drift_mm"],
                    "end_heading_error_deg": metrics["end_heading_error_deg"],
                    "quality": quality,
                    "notes": notes,
                },
                "threshold_evaluation": {
                    "pass_fail": "pass",
                    "failed_metrics": [],
                    "metrics": metrics,
                },
            }

        stable_runs = [
            make_run(
                run,
                {
                    "actual_travel_error_mm": 0.0,
                    "lateral_drift_mm": 1.0,
                    "end_heading_error_deg": 0.5,
                },
            )
            for run in plan.runs
        ]
        self.assertEqual(bench.derive_session_judgment(plan, stable_runs)["quality_flag"], "none")

        bad_grade_runs = list(stable_runs)
        bad_grade_runs[1] = make_run(
            plan.runs[1],
            {
                "actual_travel_error_mm": 0.0,
                "lateral_drift_mm": 1.0,
                "end_heading_error_deg": 0.5,
            },
            quality="bad",
        )
        self.assertEqual(
            bench.derive_session_judgment(plan, bad_grade_runs)["quality_flag"], "concern"
        )

        abnormal_note_runs = list(stable_runs)
        abnormal_note_runs[1] = make_run(
            plan.runs[1],
            {
                "actual_travel_error_mm": 0.0,
                "lateral_drift_mm": 1.0,
                "end_heading_error_deg": 0.5,
            },
            notes="visible slip near the end",
        )
        self.assertEqual(
            bench.derive_session_judgment(plan, abnormal_note_runs)["quality_flag"], "concern"
        )

        spread_runs = [
            make_run(
                plan.runs[0],
                {
                    "actual_travel_error_mm": -11.0,
                    "lateral_drift_mm": 1.0,
                    "end_heading_error_deg": 0.5,
                },
            ),
            make_run(
                plan.runs[1],
                {
                    "actual_travel_error_mm": 0.0,
                    "lateral_drift_mm": 1.0,
                    "end_heading_error_deg": 0.5,
                },
            ),
            make_run(
                plan.runs[2],
                {
                    "actual_travel_error_mm": 11.0,
                    "lateral_drift_mm": 1.0,
                    "end_heading_error_deg": 0.5,
                },
            ),
        ]
        self.assertEqual(bench.derive_session_judgment(plan, spread_runs)["quality_flag"], "concern")


if __name__ == "__main__":
    unittest.main()
