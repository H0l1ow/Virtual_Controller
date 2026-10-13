import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from vcml.dataset import assign_splits, build, session_windows
from vcml.evaluation import classification, decisions, event_proxies
from vcml.recordings import read_frames
from vcml.schema import FEATURE_COUNT, LABELS, WINDOW_SIZE


def make_hand(label, track=7, tracked=True, side="LEFT"):
    # Geometrically plausible enough to exercise feature/schema contracts;
    # these samples are ONLY test fixtures, not training data.
    points = []
    for i in range(21):
        x = .32 + ((i % 4) * .033) + (i // 4) * .009
        y = .27 + ((i % 5) * .049) + (i // 5) * .006
        points.append([x, y, .012 * (i % 3)])
    return {'side': side, 'reported_side': side, 'tracked': tracked,
            'track_id': track if tracked else 0, 'handedness_confidence': .95,
            'label': label, 'landmarks': points if tracked else []}


def make_record(path, labels, sides='left'):
    with path.open('w', encoding='utf-8') as stream:
        for i, label in enumerate(labels):
            left = make_hand(label)
            right = make_hand('UNLABELED', tracked=False, side='RIGHT')
            frame = {'schema': 'vc.gestures.jsonl.v1', 'sequence': i + 1,
                     'timestamp_us': 1_000_000 + i * 33333,
                     'width': 640, 'height': 480,
                     'hands': [left, right]}
            stream.write(json.dumps(frame) + '\n')


class M6DataTests(unittest.TestCase):
    def test_windowing_resets_on_label_boundary(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'session.jsonl'
            make_record(path, ['FIST'] * 18 + ['PINCH'] * 42)
            windows = session_windows(path, 'session', 'person')
            self.assertTrue(windows)
            self.assertEqual({s.label for s in windows}, {LABELS.index('FIST'), LABELS.index('PINCH')})
            self.assertNotEqual(windows[0].segment, windows[-1].segment)
            self.assertTrue(all(s.x.shape == (WINDOW_SIZE, FEATURE_COUNT) for s in windows))
            self.assertTrue(all(np.isfinite(s.x).all() for s in windows))

    def test_split_prevents_subject_leakage_and_is_repeatable(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            files = []
            for i in range(9):
                path = base / f'session{i}.jsonl'
                make_record(path, [LABELS[i % len(LABELS)]] * 46)
                files.append(path)
            manifest = base / 'subjects.csv'
            with manifest.open('w', newline='') as f:
                wr = csv.writer(f)
                wr.writerow(['file', 'subject_id'])
                for i, path in enumerate(files):
                    wr.writerow([path.name, f'person_{i // 2}'])
            out = base / 'data.npz'
            report1 = build(base, out, manifest, stride=3)
            report2 = build(base, out, manifest, stride=3)
            self.assertEqual(report1['sessionSplits'], report2['sessionSplits'])
            self.assertEqual(report1['splitUnit'], 'subject')
            with np.load(out) as data:
                self.assertEqual(set(data['split'].tolist()), {0, 1, 2})
                self.assertTrue(np.isfinite(data['X']).all())
                groups = data['subject']
                for g in np.unique(groups):
                    self.assertEqual(len(np.unique(data['split'][groups == g])), 1)
                self.assertEqual(data['rule_scores'].shape, (len(data['y']), 5))

    def test_subject_manifest_required_for_every_session(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            for i in range(3):
                make_record(root / f'{i}.jsonl', ['NONE'] * 42)
            (root / 'subjects.csv').write_text('file,subject_id\n0.jsonl,alice\n')
            with self.assertRaisesRegex(ValueError, 'lacks'):
                build(root, root / 'data.npz', root / 'subjects.csv')

    def test_less_than_three_groups_refused(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            make_record(root / 'single.jsonl', ['FIST'] * 46)
            with self.assertRaisesRegex(ValueError, 'three independent'):
                build(root, root / 'x.npz')

    def test_reader_rejects_invalid_tracking_and_timestamp(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'bad.jsonl'
            make_record(path, ['FIST'] * 42)
            records = path.read_text().splitlines()
            frame = json.loads(records[0])
            frame['hands'][0]['track_id'] = 0
            records[0] = json.dumps(frame)
            path.write_text('\n'.join(records) + '\n')
            with self.assertRaisesRegex(ValueError, 'track_id'):
                list(read_frames(path))

    def test_threshold_and_events_contract(self):
        raw = np.array([[.05, .95, 0, 0, 0], [.7, .3, 0, 0, 0]], dtype=np.float32)
        np.testing.assert_array_equal(decisions(raw), [1, 0])
        self.assertEqual(classification(np.array([1, 0]), np.array([1, 0]))['macroF1'], 1.0)
        result = event_proxies(np.array([1] * 6), np.array([1] * 6),
                               np.array([0] * 6), np.array([0] * 6),
                               np.array([1] * 6), np.arange(6) * 33333 + 1_000_000)
        self.assertEqual(result['gestureSegmentsHit'], 1)
        self.assertEqual(result['gestureSegmentsMissed'], 0)


if __name__ == '__main__':
    unittest.main()
