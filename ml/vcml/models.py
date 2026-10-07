from __future__ import annotations

import torch
from torch import nn
from torch.nn import functional as F

from .schema import FEATURE_COUNT, LABELS


class CausalBlock(nn.Module):
    def __init__(self, channels: int, dilation: int, dropout: float) -> None:
        super().__init__()
        self.left_padding = 2 * dilation
        self.conv = nn.Conv1d(
            channels,
            channels,
            kernel_size=3,
            dilation=dilation,
        )
        self.dropout = nn.Dropout(dropout)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        residual = self.conv(F.pad(x, (self.left_padding, 0)))
        return x + self.dropout(F.relu(residual))


class TCN(nn.Module):
    def __init__(self, hidden: int = 64, dropout: float = 0.1) -> None:
        super().__init__()
        self.input = nn.Conv1d(FEATURE_COUNT, hidden, 1)
        self.blocks = nn.Sequential(
            CausalBlock(hidden, 1, dropout),
            CausalBlock(hidden, 2, dropout),
            CausalBlock(hidden, 4, dropout),
        )
        self.head = nn.Conv1d(hidden, len(LABELS), 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = F.relu(self.input(x.transpose(1, 2)))
        x = self.blocks(x)
        return self.head(x)[:, :, -1]


class GestureModel(nn.Module):
    """Normalization is embedded so C++ sends raw vc.hand134.v2 features."""

    def __init__(
        self,
        hidden: int = 64,
        dropout: float = 0.1,
        mean=None,
        std=None,
    ) -> None:
        super().__init__()
        self.register_buffer(
            "mean",
            torch.zeros(FEATURE_COUNT)
            if mean is None
            else torch.as_tensor(mean, dtype=torch.float32),
        )
        self.register_buffer(
            "std",
            torch.ones(FEATURE_COUNT)
            if std is None
            else torch.as_tensor(std, dtype=torch.float32),
        )
        self.network = TCN(hidden, dropout)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        normalized = torch.clamp((x - self.mean) / self.std, -10.0, 10.0)
        return self.network(normalized)
