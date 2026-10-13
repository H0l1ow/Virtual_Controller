"""Inspect/replay M5 landmark JSONL sessions without a camera or Qt runtime.

Playback here is an offline landmark-stream reader, NOT system input or a
virtual camera. --csv exports raw image-space landmarks for later training.
"""

from __future__ import annotations

import argparse
import csv
import sys
import time
from collections import Counter
from pathlib import Path

# Single validator shared by playback and ML dataset preparation.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from vcml.recordings import SCHEMA, SIDES, LABELS, read_frames


def playback(source: Path, speed: float = 0.0, csv_path: Path | None = None,
             quiet: bool = False) -> tuple[int, Counter]:
    """Read all frames; optionally emit timestamped samples at recorded rate."""
    counts: Counter = Counter()
    writer = None
    target = None
    if csv_path is not None:
        csv_path.parent.mkdir(parents=True, exist_ok=True)
        target = csv_path.open("w", newline="", encoding="utf-8")
        fields = ["sequence", "timestamp_us", "width", "height", "side", "track_id", "label", "tracked"]
        fields += [f"{axis}{i}" for i in range(21) for axis in ("x", "y", "z")]
        writer = csv.writer(target)
        writer.writerow(fields)
    prior: int | None = None
    frames = 0
    try:
        for frame in read_frames(source):
            ts = frame["timestamp_us"]
            if prior is not None and speed > 0:
                delay = min((ts - prior) / 1_000_000 / speed, 2.0)
                time.sleep(delay)
            prior = ts
            frames += 1
            if not quiet:
                hand_summary = ", ".join(
                    f"{h['side']}={h['label'] if h['tracked'] else 'LOST'}"
                    for h in frame["hands"]
                )
                print(f"{frame['sequence']:>6} @ {ts}: {hand_summary}")
            for hand in frame["hands"]:
                counts[(hand["side"], hand["label"] if hand["tracked"] else "LOST")] += 1
                if writer:
                    coords = [v for point in hand["landmarks"] for v in point]
                    coords.extend([""] * (63 - len(coords)))
                    writer.writerow([frame["sequence"], ts, frame["width"], frame["height"],
                                     hand["side"], hand["track_id"], hand["label"], hand["tracked"],
                                     *coords])
    finally:
        if target:
            target.close()
    return frames, counts


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="M5 .jsonl session")
    parser.add_argument("--speed", type=float, default=0.0,
                        help="Playback speed: 0=instant, 1=recorded time, 2=2x")
    parser.add_argument("--csv", type=Path, help="Write one row per hand/frame")
    parser.add_argument("--quiet", action="store_true", help="Only show counts")
    args = parser.parse_args(argv)
    if args.speed < 0:
        parser.error("--speed cannot be negative")
    try:
        frames, counts = playback(args.input, args.speed, args.csv, args.quiet)
    except (OSError, ValueError) as error:
        print(f"Replay failed: {error}", file=sys.stderr)
        return 1
    print(f"Read {frames} frames from {args.input}")
    for (side, label), count in sorted(counts.items()):
        print(f"  {side:<5} {label:<12} {count}")
    if args.csv:
        print(f"Exported raw landmarks to {args.csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
