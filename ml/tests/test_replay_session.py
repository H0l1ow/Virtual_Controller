import csv
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "replay_session.py"
SPEC = importlib.util.spec_from_file_location("vc_replay_session", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ReplaySessionTests(unittest.TestCase):
    def sample(self):
        return {
            "schema": "vc.gestures.jsonl.v1",
            "sequence": 1,
            "timestamp_us": 1_000_000,
            "width": 640,
            "height": 480,
            "hands": [
                {"side": "LEFT", "reported_side": "LEFT", "tracked": True,
                 "track_id": 2, "handedness_confidence": .97, "label": "PINCH",
                 "landmarks": [[0.1, 0.2, 0.0] for _ in range(21)]},
                {"side": "RIGHT", "reported_side": "RIGHT", "tracked": False,
                 "track_id": 0, "handedness_confidence": 0.0, "label": "UNLABELED",
                 "landmarks": []}
            ]}

    def test_playback_and_csv(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.jsonl"
            path.write_text(json.dumps(self.sample()) + "\n")
            output = Path(tmp) / "test.csv"
            frames, counts = MODULE.playback(path, csv_path=output, quiet=True)
            self.assertEqual(frames, 1)
            self.assertEqual(counts[("LEFT", "PINCH")], 1)
            self.assertEqual(counts[("RIGHT", "LOST")], 1)
            with output.open(newline="") as source:
                rows = list(csv.DictReader(source))
            self.assertEqual(len(rows), 2)
            self.assertEqual(rows[0]["x0"], "0.1")
            self.assertEqual(rows[1]["x0"], "")

    def test_invalid_schema_or_landmarks(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.jsonl"
            frame = self.sample()
            frame["hands"][0]["landmarks"] = []
            path.write_text(json.dumps(frame) + "\n")
            with self.assertRaisesRegex(ValueError, "landmark count"):
                list(MODULE.read_frames(path))

    def test_non_monotonic_timestamps(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.jsonl"
            frame = self.sample()
            path.write_text(json.dumps(frame) + "\n" + json.dumps(frame) + "\n")
            with self.assertRaisesRegex(ValueError, "non-monotonic"):
                list(MODULE.read_frames(path))


if __name__ == "__main__":
    unittest.main()
