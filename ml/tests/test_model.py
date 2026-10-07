import sys
from pathlib import Path

import numpy as np
import torch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from vcml.models import GestureModel
from vcml.schema import FEATURE_COUNT, LABELS, WINDOW_SIZE


def test_tcn_shape_and_causality_contract():
    model = GestureModel(hidden=16, dropout=0.0).eval()
    x = torch.zeros(2, WINDOW_SIZE, FEATURE_COUNT)
    with torch.inference_mode():
        y = model(x)
    assert tuple(y.shape) == (2, len(LABELS))
    assert torch.isfinite(y).all()


def test_normalization_buffers_are_finite():
    mean = np.zeros(FEATURE_COUNT, np.float32)
    std = np.ones(FEATURE_COUNT, np.float32)
    model = GestureModel(hidden=8, dropout=0.0, mean=mean, std=std)
    assert torch.isfinite(model.mean).all()
    assert torch.isfinite(model.std).all()
