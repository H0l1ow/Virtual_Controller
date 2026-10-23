"""Validated M5 JSONL hand-landmark recordings; no images or system input."""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Iterator

from .schema import RECORDING_LABELS

SCHEMA = "vc.gestures.jsonl.v1"
SIDES = ("LEFT", "RIGHT")
LABELS = {"UNLABELED", *RECORDING_LABELS}
MAX_BYTES = 100 * 1024 * 1024
MAX_FRAMES = 18_000
MAX_LINE = 32_768


def read_frames(path: Path) -> Iterator[dict]:
    """Fail closed on incompatible schema, invalid points or timestamps."""
    if not path.is_file() or path.stat().st_size > MAX_BYTES:
        raise ValueError(f"{path}: nonexistent recording or exceeds 100 MiB")
    previous_timestamp: int | None = None
    with path.open("r", encoding="utf-8") as source:
        for lineno, line in enumerate(source, 1):
            if lineno > MAX_FRAMES:
                raise ValueError(f"{path}: recording exceeds {MAX_FRAMES} frames")
            if len(line.encode("utf-8")) > MAX_LINE:
                raise ValueError(f"{path}:{lineno}: oversized JSONL line")
            if not line.strip():
                continue
            try:
                frame = json.loads(line)
                if not isinstance(frame, dict) or frame.get("schema") != SCHEMA:
                    raise ValueError("unsupported schema")
                ts = frame.get("timestamp_us")
                if type(ts) is not int or ts <= 0:
                    raise ValueError("timestamp_us must be a positive integer")
                if previous_timestamp is not None and ts <= previous_timestamp:
                    raise ValueError("non-monotonic timestamps")
                previous_timestamp = ts
                for field in ("width", "height"):
                    value = frame.get(field)
                    if type(value) is not int or value <= 0 or value > 8192:
                        raise ValueError(f"invalid {field}")
                hands = frame.get("hands")
                if not isinstance(hands, list) or len(hands) != 2:
                    raise ValueError("expected two side entries")
                for i, hand in enumerate(hands):
                    if not isinstance(hand, dict) or hand.get("side") != SIDES[i]:
                        raise ValueError("invalid hand ordering")
                    if hand.get("label") not in LABELS:
                        raise ValueError("unknown label")
                    tracked = hand.get("tracked")
                    if type(tracked) is not bool:
                        raise ValueError("tracked must be boolean")
                    track_id = hand.get("track_id")
                    if type(track_id) is not int or (tracked and track_id <= 0):
                        raise ValueError("invalid track_id")
                    confidence = hand.get("handedness_confidence", 0.0)
                    if type(confidence) not in (float, int) or not math.isfinite(confidence) or not 0 <= confidence <= 1:
                        raise ValueError("invalid handedness confidence")
                    points = hand.get("landmarks")
                    if not isinstance(points, list) or len(points) != (21 if tracked else 0):
                        raise ValueError("invalid landmark count")
                    for point in points:
                        if not isinstance(point, list) or len(point) != 3:
                            raise ValueError("invalid landmark shape")
                        if any(v is not None and (type(v) not in (int, float) or not math.isfinite(v))
                               for v in point):
                            raise ValueError("invalid landmark value")
                yield frame
            except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
                raise ValueError(f"{path}:{lineno}: {error}") from error
