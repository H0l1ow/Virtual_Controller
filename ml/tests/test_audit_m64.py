"""M6.4 recording audit regression tests; synthetic points are ONLY fixtures."""
from __future__ import annotations

import contextlib
import csv
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from vcml.audit import audit, main
from vcml.dataset import session_windows
from vcml.schema import LABELS


def hand(label: str, side: str, *, tracked=True, invalid=False) -> dict:
    # Test fixture replicates the geometry used by the M6 tests, not real ML data.
    points = [[.32 + (i % 4) * .033 + (i // 4) * .009,
               .27 + (i % 5) * .049 + (i // 5) * .006,
               .012 * (i % 3)] for i in range(21)]
    if invalid:
        points[0][0] = 9.0
    return {"side": side, "reported_side": side, "tracked": tracked,
            "track_id": 7 if tracked else 0, "handedness_confidence": .95,
            "label": label, "landmarks": points if tracked else []}


def recording(path: Path, labels: list[str], *, side="LEFT", invalid_schema=False) -> None:
    with path.open("w", encoding="utf-8") as out:
        for i, label in enumerate(labels):
            hands = [hand("UNLABELED", "LEFT", tracked=False),
                     hand("UNLABELED", "RIGHT", tracked=False)]
            hands[0 if side == "LEFT" else 1] = hand(label, side)
            frame = {"schema": "invalid.schema" if invalid_schema else "vc.gestures.jsonl.v1",
                     "sequence": i + 1, "timestamp_us": 1_000_000 + i * 33_333,
                     "width": 640, "height": 480, "hands": hands}
            out.write(json.dumps(frame) + "\n")


class AuditTests(unittest.TestCase):
    def test_extended_static_gestures_record_without_fake_five_class_training(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "extended.jsonl"
            recording(path, ["VICTORY"] * 30)
            result = audit(path)
            self.assertEqual(result["validRecordingCount"], 1)
            self.assertEqual(result["capturedExperimentalLabelFrames"]["VICTORY"], 30)
            self.assertEqual(result["sessions"][0]["hands"]["LEFT"]
                             ["capturedLabelFrames"]["VICTORY"], 30)
            self.assertIn("EXTENDED_LABELS_RECORD_ONLY",
                          [issue["code"] for issue in result["issues"]])
            with self.assertRaisesRegex(ValueError, "five-class ONNX contract"):
                session_windows(path, "extended.jsonl", "subject1")

    def test_empty_directory_reports_actionable_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = audit(Path(tmp))
        self.assertFalse(result["readyForDatasetBuild"])
        self.assertEqual(result["recordingCount"], 0)
        self.assertIn("NO_RECORDINGS", [i["code"] for i in result["issues"]])

    def test_window_count_is_identical_to_actual_dataset_builder(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "session.jsonl"
            # 19 valid frames, a label change, then another sequence.
            recording(path, ["FIST"] * 19 + ["POINT"] * 30)
            result = audit(path)
            samples = session_windows(path, "session.jsonl", "session.jsonl")
        counts = result["trainableWindows"]
        self.assertEqual(sum(counts.values()), len(samples))
        self.assertEqual(counts["FIST"], sum(s.label == LABELS.index("FIST") for s in samples))
        self.assertEqual(counts["POINT"], sum(s.label == LABELS.index("POINT") for s in samples))
        self.assertEqual(result["validRecordingCount"], 1)
        self.assertIn("NO_SUBJECT_MANIFEST", [i["code"] for i in result["issues"]])

    def test_three_subjects_have_all_five_classes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            # Each session has >16 windows per class; mix left & right to test both.
            labels = [name for name in LABELS for _ in range(28)]
            for i in range(3):
                recording(root / f"p{i}.jsonl", labels, side="LEFT" if i % 2 == 0 else "RIGHT")
            manifest = root / "subjects.csv"
            with manifest.open("w", newline="") as out:
                writer = csv.writer(out)
                writer.writerow(["file", "subject_id"])
                for i in range(3):
                    writer.writerow([f"p{i}.jsonl", f"person_{i}"])
            result = audit(root, manifest, min_windows_per_class=0)
        self.assertTrue(result["readyForDatasetBuild"])
        self.assertTrue(result["readyForFiveClassHeldOutEvaluation"])
        self.assertEqual(result["splitGrouping"], "subject")
        self.assertEqual(result["activeSplitGroups"], 3)
        self.assertTrue(all(result["groupsPerClass"][name] == 3 for name in LABELS))
        self.assertNotIn("NO_SUBJECT_MANIFEST", [i["code"] for i in result["issues"]])

    def test_repeat_sessions_same_subject_are_one_group(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            recording(root / "one.jsonl", ["NONE"] * 32)
            recording(root / "two.jsonl", ["FIST"] * 32)
            manifest = root / "subjects.csv"
            manifest.write_text("file,subject_id\none.jsonl,person_A\ntwo.jsonl,person_A\n")
            result = audit(root, manifest)
        self.assertEqual(result["activeSplitGroups"], 1)
        self.assertFalse(result["readyForDatasetBuild"])
        self.assertIn("INSUFFICIENT_GROUPS", [i["code"] for i in result["issues"]])

    def test_invalid_schema_blocks_readiness_and_does_not_emit_landmarks(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            recording(root / "invalid.jsonl", ["FIST"] * 22, invalid_schema=True)
            result = audit(root)
        self.assertEqual(result["validRecordingCount"], 0)
        self.assertEqual(len(result["invalidRecordings"]), 1)
        self.assertFalse(result["readyForDatasetBuild"])
        self.assertNotIn('"landmarks"', json.dumps(result))

    def test_schema_compatible_missing_confidence_is_rejected_as_geometry_not_crash(self):
        # The JSONL reader explicitly permits missing confidence (default 0).
        # The feature extractor and auditor must not assume the key exists.
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "missing_conf.jsonl"
            recording(path, ["FIST"] * 22)
            frames = [json.loads(line) for line in path.read_text().splitlines()]
            for frame in frames:
                del frame["hands"][0]["handedness_confidence"]
            path.write_text("\n".join(json.dumps(f) for f in frames) + "\n")
            result = audit(path)
            samples = session_windows(path, path.name, path.name)
        self.assertEqual(samples, [])
        self.assertEqual(result["validRecordingCount"], 1)
        self.assertEqual(result["sessions"][0]["hands"]["LEFT"]["invalidGeometryFrames"], 22)

    def test_too_short_session_has_zero_trainable_windows(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            recording(root / "short.jsonl", ["OPEN_HAND"] * 6)
            result = audit(root)
        self.assertEqual(result["trainableWindows"]["OPEN_HAND"], 0)
        self.assertIn("NO_TRAINABLE_WINDOWS", [i["code"] for i in result["issues"]])

    def test_cli_writes_report_and_exits_nonzero_for_incomplete_dataset(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            recording(root / "session.jsonl", ["FIST"] * 35)
            report_path = root / "reports" / "audit.json"
            with contextlib.redirect_stdout(io.StringIO()) as captured:
                status = main(["--input", str(root), "--out", str(report_path)])
            self.assertEqual(status, 2)
            self.assertIn("Ready for dataset build: no", captured.getvalue())
            report = json.loads(report_path.read_text(encoding="utf-8"))
            self.assertEqual(report["schema"], "vc.dataset_audit.v1")
            self.assertFalse(report["readyForDatasetBuild"])


if __name__ == "__main__":
    unittest.main()
