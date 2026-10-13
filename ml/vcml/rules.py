"""Offline reproduction of M5.5 ruleGestureScores for comparable test baselines.

Scores are geometric heuristics, NOT calibrated posterior probabilities.
"""
from __future__ import annotations

import numpy as np

from .features import Geometry
from .schema import LABELS


def _linear(value: float, low: float, high: float) -> float:
    return float(np.clip((value - low) / (high - low), 0.0, 1.0))


def scores(g: Geometry, pinch_low: float = .17, pinch_high: float = .43) -> np.ndarray:
    values = np.zeros(len(LABELS), dtype=np.float32)
    values[0] = 1.0
    if not g.valid:
        return values
    c = sorted(float(v) for v in g.curl)
    other = [float(v) for v in g.curl[1:]]
    robust_min = .75 * c[0] + .25 * c[1]
    robust_max = .75 * c[3] + .25 * c[2]
    robust_other = .75 * min(other) + .25 * sorted(other)[1]
    pinch = 1 - _linear(float(g.pinch[0]), pinch_low, pinch_high)
    fist = _linear(robust_min, .27, .51)
    opened = 1 - _linear(robust_max, .05, .26)
    point = (1 - _linear(float(g.curl[0]), .08, .24)) * _linear(robust_other, .24, .48)
    if fist > .55:
        opened = point = pinch = 0.0
    elif pinch > .55:
        opened = point = 0.0
    elif point > .55:
        opened = 0.0
    values[1:] = [fist, opened, point, pinch]
    values[0] = 1 - max(values[1:])
    return values
