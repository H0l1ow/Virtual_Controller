"""Held-out M6 static-gesture evaluation, including offline Rules baseline.

The temporal event metrics are explicitly *proxies* calculated from classifier
outputs; they do not claim to measure Windows SendInput latency/false clicks.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
import torch

from .models import GestureModel
from .schema import LABELS, WINDOW_SIZE, FEATURE_COUNT
from .train import load_dataset, macro_f1


def confusion(true: np.ndarray, predicted: np.ndarray) -> np.ndarray:
    result = np.zeros((len(LABELS), len(LABELS)), dtype=np.int64)
    np.add.at(result, (true, predicted), 1)
    return result


def classification(true: np.ndarray, predicted: np.ndarray) -> dict:
    cm = confusion(true, predicted)
    result = {}
    for i, label in enumerate(LABELS):
        tp = int(cm[i, i])
        support = int(cm[i].sum())
        guesses = int(cm[:, i].sum())
        precision = tp / guesses if guesses else 0.0
        recall = tp / support if support else 0.0
        f1 = 2 * precision * recall / (precision + recall) if precision + recall else 0.0
        result[label] = dict(precision=precision, recall=recall, f1=f1, support=support)
    none_count = int(cm[0].sum())
    false_positive = int(cm[0, 1:].sum())
    return {
        'accuracy': float(np.trace(cm) / max(1, cm.sum())),
        'macroF1': macro_f1(true, predicted),
        'confusionMatrix': cm.tolist(),
        'classMetrics': result,
        'noneFalsePositiveRate': false_positive / none_count if none_count else None,
    }


def decisions(scores: np.ndarray, threshold: float = .80) -> np.ndarray:
    """M5's strict non-NONE threshold applied to model softmax or Rules scores."""
    if scores.ndim != 2 or scores.shape[1] != len(LABELS) or not np.isfinite(scores).all():
        raise ValueError('Expected finite [N,5] classifier scores')
    best = np.argmax(scores[:, 1:], axis=1) + 1
    values = scores[np.arange(len(scores)), best]
    return np.where((values >= threshold) & (values >= scores[:, 0]), best, 0).astype(np.int64)


def event_proxies(y: np.ndarray, predicted: np.ndarray, session: np.ndarray,
                  side: np.ndarray, segment: np.ndarray, timestamp: np.ndarray,
                  debounce_ms: int = 120) -> dict:
    """One PRESS candidate after a continuous stable class; no action is sent.

    Events are observed only after the causal 16-frame window is available.
    Reported latencies are since the *first available window* in a segment,
    not true first-camera-frame or physical hand-motion latency.
    """
    groups: dict[tuple[int, int, int], list[int]] = defaultdict(list)
    for i in range(len(y)):
        groups[(int(session[i]), int(side[i]), int(segment[i]))].append(i)
    hits = missed = false = 0
    delays = []
    none_seconds = 0.0
    for group in groups.values():
        group.sort(key=lambda i: int(timestamp[i]))
        truth = int(y[group[0]])
        if any(int(y[i]) != truth for i in group):
            raise ValueError('A segment must have one ground truth label')
        candidate = 0
        since = 0
        already_pressed = set()
        hit_timestamp = None
        for i in group:
            pred = int(predicted[i])
            t = int(timestamp[i])
            if pred == 0:
                candidate = 0
                continue
            if pred != candidate:
                candidate, since = pred, t
            elif t - since >= debounce_ms * 1000 and pred not in already_pressed:
                already_pressed.add(pred)
                if pred == truth and hit_timestamp is None:
                    hit_timestamp = t
                    hits += 1
                elif pred != truth:
                    false += 1
        if truth != 0 and hit_timestamp is None:
            missed += 1
        if hit_timestamp is not None:
            delays.append((hit_timestamp - int(timestamp[group[0]])) / 1000.0)
        if truth == 0:
            none_seconds += (int(timestamp[group[-1]]) - int(timestamp[group[0]])) / 1e6 + 1 / 30
    return {
        'gestureSegmentsHit': hits, 'gestureSegmentsMissed': missed,
        'wrongClassOrNoneSegmentPresses': false,
        'falsePressesPerMinuteOfNONE': false / (none_seconds / 60) if none_seconds > 0 else None,
        'medianPressDelayAfterWarmupMs': float(np.median(delays)) if delays else None,
        'note': 'Offline debounce proxy; does not replay GestureStateManager cooldown/grace or SendInput.',
    }


def probabilities(model: GestureModel, x: np.ndarray, device: str = 'cpu') -> np.ndarray:
    model.eval().to(device)
    values = []
    with torch.inference_mode():
        for start in range(0, len(x), 256):
            logits = model(torch.from_numpy(x[start:start+256]).to(device))
            values.append(torch.softmax(logits, dim=1).cpu().numpy())
    return np.concatenate(values).astype(np.float32)


def load_model(run: Path) -> GestureModel:
    checkpoint = torch.load(run / 'best.pt', map_location='cpu', weights_only=True)
    config = checkpoint['config']
    if config.get('windowSize') != WINDOW_SIZE or config.get('featureCount') != FEATURE_COUNT or config.get('labels') != LABELS:
        raise ValueError('Checkpoint has incompatible window, feature or label contract')
    model = GestureModel(config['hidden'], config['dropout'])
    model.load_state_dict(checkpoint['model'])
    return model.eval()


def evaluate_run(dataset: Path, run: Path, output: Path, split: str = 'test',
                 threshold: float = .80, device: str = 'cpu',
                 allow_dataset_change: bool = False) -> dict:
    x, y, splits = load_dataset(dataset)
    config = json.loads((run / 'config.json').read_text(encoding='utf-8'))
    digest = hashlib.sha256(dataset.read_bytes()).hexdigest()
    if not allow_dataset_change and config.get('datasetSha256') != digest:
        raise ValueError('Dataset hash mismatch with training run; use the original dataset')
    target = {'train': 0, 'validation': 1, 'test': 2}[split]
    mask = splits == target
    if not np.any(mask):
        raise ValueError(f'{split} split has no windows')
    with np.load(dataset, allow_pickle=False) as data:
        required = {'rule_scores', 'session', 'subject', 'side', 'segment', 'timestamp_us'}
        if not required.issubset(data.files):
            raise ValueError('M6 dataset with rule scores and session/timestamps required')
        rules = np.asarray(data['rule_scores'], np.float32)[mask]
        sessions = np.asarray(data['session'])[mask]
        subjects = np.asarray(data['subject'])[mask]
        sides = np.asarray(data['side'])[mask]
        segments = np.asarray(data['segment'])[mask]
        times = np.asarray(data['timestamp_us'])[mask]
    neural = probabilities(load_model(run), x[mask], device)
    outputs = {}
    for name, values in [('TCN', neural), ('Rules', rules)]:
        predicted = decisions(values, threshold)
        outputs[name] = classification(y[mask], predicted)
        outputs[name]['events'] = event_proxies(y[mask], predicted, sessions, sides, segments, times)
    report = {
        'model': 'causal_tcn', 'labels': LABELS, 'split': split, 'threshold': threshold,
        'testedSubjects': len(set(subjects.tolist())), 'testedSessions': len(set(sessions.tolist())),
        'windowCount': int(mask.sum()), 'datasetSha256': digest,
        'models': outputs,
        'limitations': [
            'Rules are calculated offline from same M5 geometry; thresholds use default 0.80, not per-user calibrated overrides.',
            'Offline label samples are user annotated; no assertion of label quality is made.',
            'Overlapping window predictions are correlated; window metrics are not independent observations.',
            'Event proxies omit full GestureStateManager grace/cooldown and hardware interaction.',
            'No dynamic swipe classes: 134 v2 features suppress translation of the entire hand.',
        ],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description='Compare causal TCN vs M5 Rules on held-out recordings')
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--run', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--split', choices=['validation', 'test'], default='test')
    parser.add_argument('--threshold', type=float, default=.8)
    parser.add_argument('--device', default='cpu')
    args = parser.parse_args(argv)
    if not 0 < args.threshold <= 1:
        parser.error('--threshold must be in (0,1]')
    print(json.dumps(evaluate_run(args.data, args.run, args.out, args.split, args.threshold, args.device), indent=2))


if __name__ == '__main__':
    main()
