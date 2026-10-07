from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import numpy as np

from .schema import FEATURE_COUNT, MAX_GAP_US, SAMPLE_RATE, WINDOW_SIZE


@dataclass
class Geometry:
    local: np.ndarray
    palm: np.ndarray
    scale: float
    curl: np.ndarray
    pinch: np.ndarray
    valid: bool


def geometry(hand: dict, width: int, height: int) -> Geometry:
    empty = Geometry(
        np.zeros((21, 3), np.float32),
        np.zeros(3, np.float32),
        0.0,
        np.zeros(4, np.float32),
        np.zeros(2, np.float32),
        False,
    )

    confidence = float(hand.get("handedness_confidence", 0.0))
    if not hand.get("tracked", False) or width <= 0 or height <= 0:
        return empty
    if not np.isfinite(confidence) or confidence < 0.20:
        return empty

    points = np.asarray(hand["landmarks"], dtype=np.float32).copy()
    if points.shape != (21, 3) or not np.isfinite(points).all():
        return empty
    if np.any(points[:, :2] < -0.08) or np.any(points[:, :2] > 1.08):
        return empty
    if np.any(np.abs(points[:, 2]) > 1.5):
        return empty

    points[:, 1] *= np.float32(height / width)
    scale = np.float32(
        (np.linalg.norm(points[0] - points[9])
         + np.linalg.norm(points[5] - points[17])) * 0.5
    )
    if scale < 0.02 or scale > 0.65:
        return empty

    local = (points - points[0]) / scale
    if hand.get("side") == "left":
        local[:, 0] *= -1

    curl = np.zeros(4, np.float32)
    for finger in range(4):
        base = 5 + 4 * finger
        chain = sum(
            np.linalg.norm(points[i + 1] - points[i])
            for i in range(base, base + 3)
        )
        if chain < 0.003:
            return empty
        curl[finger] = np.clip(
            1.0 - np.linalg.norm(points[base + 3] - points[base]) / chain,
            0.0,
            1.0,
        )

    pinch = np.array(
        [
            np.linalg.norm(local[4] - local[8]),
            np.linalg.norm(local[4] - local[12]),
        ],
        np.float32,
    )

    palm = points[[0, 5, 9, 13, 17]].mean(axis=0)
    return Geometry(local, palm, float(scale), curl, pinch, True)


class FeatureExtractor:
    def __init__(self) -> None:
        self.previous: np.ndarray | None = None
        self.last_us = 0

    def reset(self) -> None:
        self.previous = None
        self.last_us = 0

    def update(self, hand: dict, width: int, height: int, t_us: int) -> np.ndarray:
        g = geometry(hand, width, height)
        out = np.zeros(FEATURE_COUNT, np.float32)
        if not g.valid:
            self.reset()
            return out

        out[:63] = g.local.reshape(-1)
        dt = np.float32(t_us - self.last_us) * np.float32(1e-6)
        if self.previous is not None and 0 < dt <= 0.15:
            out[63:126] = np.clip(
                (g.local - self.previous) / dt,
                -20.0,
                20.0,
            ).reshape(-1)

        out[126:130] = g.curl
        out[130:132] = np.clip(g.pinch, 0.0, 3.0)
        out[132] = np.clip(float(hand["handedness_confidence"]), 0.0, 1.0)
        out[133] = 1.0

        self.previous = g.local.copy()
        self.last_us = t_us
        return out


class FeatureStream:
    """Causal zero-order-hold stream matching the C++ M3 runtime contract."""

    def __init__(self, window: int = WINDOW_SIZE, rate: int = SAMPLE_RATE) -> None:
        if not 2 <= window <= 120 or not 10 <= rate <= 120:
            raise ValueError("Invalid window/rate")
        self.size = window
        self.period_us = 1_000_000 // rate
        self.extractor = FeatureExtractor()
        self.history: deque[np.ndarray] = deque(maxlen=window)
        self.previous: tuple[dict, int, int, int] | None = None
        self.next_us = 0

    def reset(self) -> None:
        self.extractor.reset()
        self.history.clear()
        self.previous = None
        self.next_us = 0

    def push(self, hand: dict, width: int, height: int, t_us: int) -> list[np.ndarray]:
        if not geometry(hand, width, height).valid:
            self.reset()
            return []
        if self.previous is not None and t_us <= self.previous[3]:
            return []
        if self.previous is not None and t_us - self.previous[3] > MAX_GAP_US:
            self.reset()

        current = (hand, width, height, t_us)
        if self.previous is None:
            self.previous = current
            self.next_us = t_us

        windows: list[np.ndarray] = []
        while self.next_us <= t_us:
            source = current if self.next_us == t_us else self.previous
            h, w, ht, _ = source
            self.history.append(self.extractor.update(h, w, ht, self.next_us))
            if len(self.history) == self.size:
                windows.append(np.stack(self.history))
            self.next_us += self.period_us

        self.previous = current
        return windows
