"""M6.0: leakage-safe causal landmark windows from real M5 JSONL sessions.

No synthetic training data are produced by this module. All splits are at the
session/subject level, never at overlapping-window level.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import random
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from .features import FeatureStream, geometry
from .recordings import read_frames
from .rules import scores as rule_scores
from .schema import FEATURE_COUNT, FEATURE_SCHEMA, LABELS, MAX_GAP_US, SAMPLE_RATE, WINDOW_SIZE


@dataclass(frozen=True)
class Sample:
    x: np.ndarray
    label: int
    session: str
    subject: str
    side: int
    segment: int
    timestamp_us: int
    rules: np.ndarray


def discover(path: Path) -> list[Path]:
    if path.is_file():
        return [path] if path.suffix.lower() == '.jsonl' else []
    return sorted(p for p in path.rglob('*.jsonl') if p.is_file()) if path.is_dir() else []


def read_subjects(path: Path | None, files: list[Path], root: Path) -> dict[str, str]:
    """CSV columns: file,subject_id. Require every input session if supplied."""
    if path is None:
        return {}
    result: dict[str, str] = {}
    with path.open(newline='', encoding='utf-8-sig') as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None or not {'file', 'subject_id'}.issubset(reader.fieldnames):
            raise ValueError('Subject manifest requires file,subject_id columns')
        for row in reader:
            key, subject = row['file'].strip().replace('\\', '/'), row['subject_id'].strip()
            if not key or not subject or key in result:
                raise ValueError('Empty or duplicate file/subject_id in manifest')
            result[key] = subject
    expected = {p.relative_to(root).as_posix() for p in files}
    missing = expected - result.keys()
    if missing:
        raise ValueError(f'Subject manifest lacks {len(missing)} recording(s): {sorted(missing)[:3]}')
    return result


def session_windows(path: Path, session: str, subject: str) -> list[Sample]:
    streams = [FeatureStream() for _ in range(2)]
    previous_label: list[str | None] = [None, None]
    previous_track = [0, 0]
    previous_ts = [0, 0]
    segment = [0, 0]
    smoothed: list[np.ndarray | None] = [None, None]
    count = [0, 0]
    result: list[Sample] = []
    for frame in read_frames(path):
        for side, hand in enumerate(frame['hands']):
            label = hand['label']
            if label not in LABELS and label != 'UNLABELED':
                raise ValueError(
                    f'{path}: {label} was recorded for a future 13-class model, '
                    'but the current dataset builder only supports the fixed '
                    'five-class ONNX contract. Separate or archive extended '
                    'sessions until the versioned 13-class trainer is available.'
                )
            track = hand['track_id'] if hand['tracked'] else 0
            ts = frame['timestamp_us']
            g = geometry({'tracked': hand['tracked'], 'landmarks': hand['landmarks'],
                          'side': 'left' if side == 0 else 'right',
                          'handedness_confidence': hand.get('handedness_confidence', 0.0)},
                         frame['width'], frame['height']) if hand['tracked'] else None
            valid = track > 0 and label in LABELS and label != 'UNLABELED' and g is not None and g.valid
            reset = (not valid or label != previous_label[side] or track != previous_track[side]
                     or (previous_ts[side] and ts - previous_ts[side] > MAX_GAP_US))
            if reset:
                streams[side].reset()
                smoothed[side] = None
                count[side] = 0
                segment[side] += 1
            previous_label[side] = label if valid else None
            previous_track[side] = track if valid else 0
            previous_ts[side] = ts
            if not valid:
                continue
            hand_record = {
                'tracked': True, 'landmarks': hand['landmarks'],
                'side': 'left' if side == 0 else 'right',
                'handedness_confidence': hand.get('handedness_confidence', 0.0),
            }
            windows = streams[side].push(hand_record, frame['width'], frame['height'], ts)
            raw = rule_scores(g)
            smoothed[side] = raw if smoothed[side] is None else (
                np.float32(.55) * smoothed[side] + np.float32(.45) * raw)
            for idx, x in enumerate(windows):
                count[side] += 1
                # The end timestamp is the fixed-rate sampling time (not necessarily capture time).
                end_us = streams[side].next_us - (len(windows) - idx) * streams[side].period_us
                result.append(Sample(x, LABELS.index(label), session, subject, side,
                                     segment[side], end_us, smoothed[side].copy()))
    return result


def assign_splits(samples: list[Sample], seed: int = 42,
                  fractions: tuple[float, float, float] = (.7, .15, .15)) -> dict[str, int]:
    if len(fractions) != 3 or min(fractions) <= 0 or not np.isclose(sum(fractions), 1):
        raise ValueError('Fractions must be three positive numbers summing to 1')
    groups = sorted({s.subject for s in samples})
    if len(groups) < 3:
        raise ValueError('At least three independent subjects/sessions are required for train/val/test')
    group_counts = {name: np.bincount([s.label for s in samples if s.subject == name],
                                      minlength=len(LABELS)) for name in groups}
    group_sizes = {name: int(values.sum()) for name, values in group_counts.items()}
    total = sum(group_sizes.values())
    target = np.asarray(fractions) * total
    best = None
    best_score = float('inf')
    # Search deterministic permutations to limit class omissions (esp. in
    # train) while preserving entire subject/session groups. This is an
    # approximate group-stratified split, never a window-level split.
    for attempt in range(256):
        shuffled = groups.copy()
        random.Random(seed + attempt).shuffle(shuffled)
        totals = [0, 0, 0]
        assignment: dict[str, int] = {}
        for i, group in enumerate(shuffled):
            remaining = len(shuffled) - i
            missing = [k for k in range(3) if k not in assignment.values()]
            choices = missing if len(missing) == remaining else [0, 1, 2]
            chosen = min(choices, key=lambda k: (totals[k] + group_sizes[group] - target[k])**2
                         - (totals[k] - target[k])**2)
            assignment[group] = chosen
            totals[chosen] += group_sizes[group]
        present = np.zeros((3, len(LABELS)), dtype=np.int64)
        for group, index in assignment.items():
            present[index] += group_counts[group]
        in_data = np.sum(present, axis=0) > 0
        in_many = np.asarray([sum(group_counts[g][cls] > 0 for g in groups) >= 3
                              for cls in range(len(LABELS))])
        # Missing a class in training is the highest-priority avoidable error.
        missing_train = int(np.sum((present[0] == 0) & in_data))
        missing_other = int(np.sum(((present[1] == 0) | (present[2] == 0)) & in_many))
        imbalance = float(sum(abs(totals[k] - target[k]) for k in range(3)) / total)
        score = 50 * missing_train + 10 * missing_other + imbalance
        if score < best_score:
            best, best_score = assignment, score
            if score < 0.02:
                break
    assert best is not None and len(set(best.values())) == 3
    return best


def build(input_path: Path, output: Path, subjects_path: Path | None = None,
          seed: int = 42, stride: int = 2) -> dict:
    if stride < 1 or stride > 60:
        raise ValueError('Stride must be between 1 and 60')
    files = discover(input_path)
    if not files:
        raise ValueError(f'No .jsonl recordings at {input_path}')
    root = input_path if input_path.is_dir() else input_path.parent
    subjects = read_subjects(subjects_path, files, root)
    records: list[Sample] = []
    sessions: list[str] = []
    rejected: list[str] = []
    for path in files:
        name = path.relative_to(root).as_posix()
        session = session_windows(path, name, subjects.get(name, name))
        # Stride within a *segment*, never across gesture/identity boundaries.
        counter: dict[tuple[int, int], int] = {}
        retained = []
        for sample in session:
            key = (sample.side, sample.segment)
            count = counter.get(key, 0)
            if count % stride == 0:
                retained.append(sample)
            counter[key] = count + 1
        session = retained
        if session:
            sessions.append(name)
            records.extend(session)
        else:
            rejected.append(name)
    if not records:
        raise ValueError('No valid labeled 16-frame windows. Record each gesture for >=0.6 s.')
    allocation = assign_splits(records, seed)
    session_index = {name: i for i, name in enumerate(sessions)}
    subject_names = sorted({v.subject for v in records})
    subject_index = {s: i for i, s in enumerate(subject_names)}
    arrays = {
        'X': np.stack([s.x for s in records]).astype(np.float32),
        'y': np.asarray([s.label for s in records], dtype=np.int64),
        'split': np.asarray([allocation[s.subject] for s in records], dtype=np.int8),
        'session': np.asarray([session_index[s.session] for s in records], dtype=np.int32),
        'subject': np.asarray([subject_index[s.subject] for s in records], dtype=np.int32),
        'side': np.asarray([s.side for s in records], dtype=np.uint8),
        'segment': np.asarray([s.segment for s in records], dtype=np.int32),
        'timestamp_us': np.asarray([s.timestamp_us for s in records], dtype=np.int64),
        'rule_scores': np.stack([s.rules for s in records]).astype(np.float32),
    }
    if not np.isfinite(arrays['X']).all() or not np.isfinite(arrays['rule_scores']).all():
        raise ValueError('Non-finite extracted features')
    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(output, **arrays)
    stats = {split: {label: int(np.sum((arrays['split'] == idx) & (arrays['y'] == j)))
                     for j, label in enumerate(LABELS)}
             for idx, split in enumerate(('train', 'validation', 'test'))}
    report = {
        'schemaVersion': 2, 'featureSchema': FEATURE_SCHEMA, 'featureCount': FEATURE_COUNT,
        'windowSize': WINDOW_SIZE, 'sampleRate': SAMPLE_RATE, 'labels': LABELS,
        'seed': seed, 'stride': stride, 'splitUnit': 'subject' if subjects_path else 'session',
        'sessions': sessions, 'subjects': subject_names,
        'sessionSplits': {s: ['train', 'validation', 'test'][allocation[subjects.get(s, s)]] for s in sessions},
        'classCounts': stats, 'samples': len(records), 'ignoredRecordings': rejected,
        'missingClassesPerSplit': {name: [k for k, n in values.items() if n == 0]
                                   for name, values in stats.items()},
        'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
        'notes': ['Overlapping windows never cross gesture, hand track or session boundaries.',
                  'Windows from the same split unit are never shared between splits.',
                  'A manifest mapping all sessions to subject IDs is recommended for multi-session users.',
                  'This dataset contains only static gestures; swipe needs a new feature schema.'],
    }
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description='M6: prepare landmark recordings into leakage-safe causal windows')
    parser.add_argument('--input', type=Path, required=True, help='JSONL recording or directory containing sessions')
    parser.add_argument('--out', type=Path, required=True, help='dataset.npz')
    parser.add_argument('--subjects', type=Path, help='Optional CSV file,subject_id for subject-safe splits')
    parser.add_argument('--stride', type=int, default=2, help='Keep every Nth window within each segment')
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args(argv)
    print(json.dumps(build(args.input, args.out, args.subjects, args.seed, args.stride), indent=2))


if __name__ == '__main__':
    main()
