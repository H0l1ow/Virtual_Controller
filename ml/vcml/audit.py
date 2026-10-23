"""Preflight M6 landmark JSONL recordings before dataset creation.

This is deliberately a *structural* audit. It does not infer the true gesture,
judge whether manual labels are correct, or claim real-world model accuracy.
It reuses the production 30-Hz feature stream to count trainable windows,
without retaining the feature arrays or any landmark coordinates in the report.
"""
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from .dataset import discover, read_subjects
from .features import FeatureStream, geometry
from .recordings import read_frames
from .schema import (FEATURE_SCHEMA, LABELS, RECORDING_LABELS,
                     MAX_GAP_US, SAMPLE_RATE, WINDOW_SIZE)

SIDES = ("LEFT", "RIGHT")
REPORT_SCHEMA = "vc.dataset_audit.v1"


def _empty_counts() -> dict[str, int]:
    return {label: 0 for label in LABELS}


def _session(path: Path, name: str, subject: str) -> dict[str, Any]:
    """Audit a single session using the same segment/reset logic as dataset.py."""
    streams = [FeatureStream() for _ in SIDES]
    previous_label: list[str | None] = [None, None]
    previous_track = [0, 0]
    previous_ts = [0, 0]
    tracked = [0, 0]
    labeled = [0, 0]
    invalid_geometry = [0, 0]
    untracked_labeled = [0, 0]
    unlabeled = [0, 0]
    resets = [0, 0]
    label_frames = [Counter(), Counter()]
    windows = [Counter(), Counter()]
    frame_count = 0
    first_ts = last_ts = 0
    camera_gaps = 0
    intervals: list[int] = []

    for frame in read_frames(path):
        ts = frame["timestamp_us"]
        if last_ts:
            delta = ts - last_ts
            intervals.append(delta)
            if delta > MAX_GAP_US:
                camera_gaps += 1
        if not first_ts:
            first_ts = ts
        last_ts = ts
        frame_count += 1
        for side, hand in enumerate(frame["hands"]):
            label = hand["label"]
            track = hand["track_id"] if hand["tracked"] else 0
            if hand["tracked"]:
                tracked[side] += 1
            if label == "UNLABELED":
                unlabeled[side] += 1
            else:
                labeled[side] += 1
                label_frames[side][label] += 1

            g = geometry({
                "tracked": hand["tracked"], "landmarks": hand["landmarks"],
                "side": "left" if side == 0 else "right",
                "handedness_confidence": hand.get("handedness_confidence", 0.0),
            }, frame["width"], frame["height"]) if hand["tracked"] else None
            geometric_valid = track > 0 and g is not None and g.valid
            # The recorder accepts 13 static labels, but this unchanged
            # ONNX/dataset version trains only the original five classes.
            valid = geometric_valid and label in LABELS
            if label != "UNLABELED" and not geometric_valid:
                if not hand["tracked"]:
                    untracked_labeled[side] += 1
                else:
                    invalid_geometry[side] += 1

            reset = (not valid or label != previous_label[side]
                     or track != previous_track[side]
                     or (previous_ts[side] and ts - previous_ts[side] > MAX_GAP_US))
            if reset:
                streams[side].reset()
                resets[side] += 1
            previous_label[side] = label if valid else None
            previous_track[side] = track if valid else 0
            previous_ts[side] = ts
            if not valid:
                continue
            for _ in streams[side].push({
                "tracked": True, "landmarks": hand["landmarks"],
                "side": "left" if side == 0 else "right",
                "handedness_confidence": hand.get("handedness_confidence", 0.0),
            }, frame["width"], frame["height"], ts):
                windows[side][label] += 1

    # Median sampling interval is a rough camera diagnostic, not the training
    # resample rate. We avoid numpy and never persist raw timestamps/landmarks.
    sorted_intervals = sorted(intervals)
    middle = len(sorted_intervals) // 2
    median_us = (float(sorted_intervals[middle]) if len(sorted_intervals) % 2
                 else (sorted_intervals[middle - 1] + sorted_intervals[middle]) / 2
                 ) if sorted_intervals else None
    side_reports = {}
    for i, side_name in enumerate(SIDES):
        side_reports[side_name] = {
            "trackedFrames": tracked[i], "labeledFrames": labeled[i],
            "unlabeledFrames": unlabeled[i],
            "untrackedLabeledFrames": untracked_labeled[i],
            "invalidGeometryFrames": invalid_geometry[i],
            "segmentResets": resets[i],
            "labelFrames": {k: int(label_frames[i][k]) for k in LABELS},
            "capturedLabelFrames": {k: int(label_frames[i][k]) for k in RECORDING_LABELS},
            "trainableWindows": {k: int(windows[i][k]) for k in LABELS},
        }
    return {
        "file": name, "subject": subject, "frames": frame_count,
        "durationSeconds": round((last_ts - first_ts) / 1e6, 3) if frame_count else 0.0,
        "medianCameraFps": round(1e6 / median_us, 2) if median_us else None,
        "captureGapsOver150ms": camera_gaps,
        "hands": side_reports,
        "totalTrainableWindows": sum(sum(c.values()) for c in windows),
    }


def audit(input_path: Path, subjects_path: Path | None = None,
          min_windows_per_class: int = 100) -> dict[str, Any]:
    """Return a JSON-serializable readiness report; never train or export models.

    `readyForDatasetBuild` expresses *technical requirements* for the existing
    builder, not training quality. It deliberately makes no performance claims.
    """
    if min_windows_per_class < 0:
        raise ValueError("min_windows_per_class must be nonnegative")
    files = discover(input_path)
    root = input_path if input_path.is_dir() else input_path.parent
    subjects = read_subjects(subjects_path, files, root) if files else {}
    sessions: list[dict[str, Any]] = []
    invalid: list[dict[str, str]] = []
    windows = Counter()
    side_windows = {side: Counter() for side in SIDES}
    group_classes: dict[str, set[str]] = defaultdict(set)
    total_frames = 0
    extended_frames = Counter()

    for file in files:
        name = file.relative_to(root).as_posix()
        subject = subjects.get(name, name)
        try:
            result = _session(file, name, subject)
        except (OSError, UnicodeError, ValueError, TypeError) as exc:
            invalid.append({"file": name, "reason": str(exc)})
            continue
        sessions.append(result)
        total_frames += result["frames"]
        for side in SIDES:
            for label in RECORDING_LABELS:
                if label not in LABELS:
                    extended_frames[label] += result["hands"][side]["capturedLabelFrames"][label]
            for label, count in result["hands"][side]["trainableWindows"].items():
                windows[label] += count
                side_windows[side][label] += count
                if count:
                    group_classes[subject].add(label)

    classes_per_group = {label: sum(label in labels for labels in group_classes.values())
                         for label in LABELS}
    active_groups = len(group_classes)
    missing = [label for label in LABELS if windows[label] == 0]
    sparse = [label for label in LABELS if 0 < windows[label] < min_windows_per_class]
    incomplete = [label for label in LABELS if 0 < classes_per_group[label] < 3]
    no_windows = [entry["file"] for entry in sessions
                  if entry["totalTrainableWindows"] == 0]
    issues = []

    def issue(level: str, code: str, detail: str) -> None:
        issues.append({"level": level, "code": code, "message": detail})

    if not files:
        issue("error", "NO_RECORDINGS", "No JSONL recordings found; record real gestures first.")
    if invalid:
        issue("error", "INVALID_RECORDINGS", f"{len(invalid)} invalid recording(s); the dataset builder cannot consume them.")
    if active_groups < 3:
        issue("error", "INSUFFICIENT_GROUPS", "At least 3 groups with usable windows are needed for train/validation/test.")
    if missing:
        issue("error", "MISSING_CLASSES", "No trainable windows for: " + ", ".join(missing))
    if incomplete:
        issue("warning", "CLASS_GROUP_COVERAGE", "Fewer than 3 independent groups contain: " + ", ".join(incomplete))
    if sparse:
        issue("warning", "LOW_CLASS_VOLUME", "Fewer than the suggested number of correlated windows for: " + ", ".join(sparse))
    if no_windows:
        issue("warning", "NO_TRAINABLE_WINDOWS", f"{len(no_windows)} valid recording(s) contain no complete {WINDOW_SIZE}-sample windows.")
    if any(extended_frames.values()):
        issue("warning", "EXTENDED_LABELS_RECORD_ONLY",
              "Experimental static gesture samples were captured successfully, "
              "but the current five-class dataset/trainer ignores these labels. "
              "A versioned 13-class training pipeline is required.")
    if files and subjects_path is None:
        issue("warning", "NO_SUBJECT_MANIFEST", "Each session is treated as a separate group; repeated recordings by one person may leak across splits.")
    for side in SIDES:
        missing_side = [label for label in LABELS if side_windows[side][label] == 0]
        if missing_side:
            issue("warning", f"NO_{side}_COVERAGE", f"No {side} windows for: " + ", ".join(missing_side))

    structural_ready = (bool(files) and not invalid and active_groups >= 3 and not missing)
    heldout_ready = structural_ready and not incomplete
    return {
        "schema": REPORT_SCHEMA, "featureSchema": FEATURE_SCHEMA,
        "windowSize": WINDOW_SIZE, "sampleRate": SAMPLE_RATE, "classes": LABELS,
        "splitGrouping": "subject" if subjects_path else "session",
        "recordingCount": len(files), "validRecordingCount": len(sessions),
        "invalidRecordings": invalid, "totalFrames": total_frames,
        "capturedExperimentalLabelFrames": {
            label: int(extended_frames[label]) for label in RECORDING_LABELS
            if label not in LABELS},
        "trainableWindows": {label: int(windows[label]) for label in LABELS},
        "trainableWindowsByHand": {side: {label: int(side_windows[side][label]) for label in LABELS}
                                   for side in SIDES},
        "activeSplitGroups": active_groups,
        "groupsPerClass": classes_per_group,
        "suggestedMinimumCorrelatedWindowsPerClass": min_windows_per_class,
        "readyForDatasetBuild": structural_ready,
        "readyForFiveClassHeldOutEvaluation": heldout_ready,
        "issues": issues, "sessions": sessions,
        "limitations": [
            "Manual gesture labels cannot be verified from landmarks alone.",
            "Overlapping trainable windows are correlated, not independent examples.",
            "Volume and group coverage do not establish real-world accuracy or safe input behavior.",
            "Dataset preparation may still reject a pathological split; inspect dataset.json after building.",
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Audit real landmark JSONL recordings before training")
    parser.add_argument("--input", type=Path, required=True, help="JSONL file or recording directory")
    parser.add_argument("--subjects", type=Path, help="CSV mapping file,subject_id (recommended)")
    parser.add_argument("--out", type=Path, help="Optional JSON report destination")
    parser.add_argument("--min-windows-per-class", type=int, default=100,
                        help="Volume advisory only; not a quality threshold")
    args = parser.parse_args(argv)
    try:
        report = audit(args.input, args.subjects, args.min_windows_per_class)
        if args.out:
            args.out.parent.mkdir(parents=True, exist_ok=True)
            args.out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError) as exc:
        parser.exit(1, f"Audit failed: {exc}\n")

    print(f"Recordings: {report['validRecordingCount']}/{report['recordingCount']}; "
          f"split groups: {report['activeSplitGroups']} ({report['splitGrouping']})")
    print("Class            Windows     LEFT    RIGHT  Groups")
    for label in LABELS:
        print(f"{label:<16} {report['trainableWindows'][label]:>7} "
              f"{report['trainableWindowsByHand']['LEFT'][label]:>8} "
              f"{report['trainableWindowsByHand']['RIGHT'][label]:>8} "
              f"{report['groupsPerClass'][label]:>7}")
    for entry in report["issues"]:
        print(f"[{entry['level'].upper()}] {entry['code']}: {entry['message']}")
    print("Ready for dataset build:", "yes" if report["readyForDatasetBuild"] else "no")
    print("Ready for five-class held-out evaluation:",
          "yes" if report["readyForFiveClassHeldOutEvaluation"] else "no")
    if args.out:
        print(f"Report: {args.out}")
    return 0 if report["readyForDatasetBuild"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
