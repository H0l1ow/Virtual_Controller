from __future__ import annotations

import argparse
import json
import random
from pathlib import Path

import numpy as np
import torch
from torch.utils.data import DataLoader, TensorDataset

from .models import GestureModel
from .schema import FEATURE_COUNT, LABELS, WINDOW_SIZE


def load_dataset(path: Path):
    with np.load(path, allow_pickle=False) as data:
        x = np.asarray(data["X"], np.float32)
        y = np.asarray(data["y"], np.int64)
        split = np.asarray(data["split"], np.int8)

    if x.ndim != 3 or x.shape[1:] != (WINDOW_SIZE, FEATURE_COUNT):
        raise ValueError(f"Expected X [N,{WINDOW_SIZE},{FEATURE_COUNT}], got {x.shape}")
    if y.shape != (len(x),) or split.shape != (len(x),):
        raise ValueError("y/split length mismatch")
    if len(x) == 0 or not np.isfinite(x).all():
        raise ValueError("Dataset is empty or contains non-finite values")
    if np.any(y < 0) or np.any(y >= len(LABELS)):
        raise ValueError("Class index outside M3 label set")
    if not all(np.any(split == value) for value in (0, 1)):
        raise ValueError("Dataset needs train split=0 and val split=1")
    return x, y, split


def macro_f1(y_true: np.ndarray, y_pred: np.ndarray) -> float:
    values = []
    for label in range(len(LABELS)):
        tp = int(np.sum((y_true == label) & (y_pred == label)))
        fp = int(np.sum((y_true != label) & (y_pred == label)))
        fn = int(np.sum((y_true == label) & (y_pred != label)))
        if tp + fp + fn == 0:
            continue
        precision = tp / max(1, tp + fp)
        recall = tp / max(1, tp + fn)
        values.append(0.0 if precision + recall == 0 else 2 * precision * recall / (precision + recall))
    return float(np.mean(values)) if values else 0.0


def evaluate(model, x, y, device: str) -> float:
    model.eval()
    predictions = []
    with torch.inference_mode():
        for start in range(0, len(x), 256):
            logits = model(torch.from_numpy(x[start:start + 256]).to(device))
            predictions.append(logits.argmax(1).cpu().numpy())
    return macro_f1(y, np.concatenate(predictions))


def train(
    dataset: Path,
    run: Path,
    epochs: int = 60,
    patience: int = 10,
    batch_size: int = 64,
    hidden: int = 64,
    dropout: float = 0.1,
    learning_rate: float = 1e-3,
    seed: int = 42,
    device: str = "cpu",
):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)

    x, y, split = load_dataset(dataset)
    train_mask = split == 0
    val_mask = split == 1

    mean = x[train_mask].reshape(-1, FEATURE_COUNT).mean(0)
    std = x[train_mask].reshape(-1, FEATURE_COUNT).std(0)
    std = np.where(std < 1e-5, 1.0, std).astype(np.float32)

    model = GestureModel(hidden, dropout, mean, std).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=learning_rate, weight_decay=1e-4)
    criterion = torch.nn.CrossEntropyLoss()

    train_set = TensorDataset(
        torch.from_numpy(x[train_mask]),
        torch.from_numpy(y[train_mask]),
    )

    run.mkdir(parents=True, exist_ok=True)
    best_f1 = -1.0
    stale = 0
    history = []

    config = {
        "schemaVersion": 2,
        "dataset": str(dataset),
        "windowSize": WINDOW_SIZE,
        "featureCount": FEATURE_COUNT,
        "labels": LABELS,
        "hidden": hidden,
        "dropout": dropout,
        "batchSize": batch_size,
        "learningRate": learning_rate,
        "seed": seed,
    }

    for epoch in range(epochs):
        generator = torch.Generator().manual_seed(seed + epoch)
        loader = DataLoader(
            train_set,
            batch_size=batch_size,
            shuffle=True,
            generator=generator,
            num_workers=0,
        )

        model.train()
        loss_sum = 0.0
        sample_count = 0
        for batch_x, batch_y in loader:
            batch_x = batch_x.to(device)
            batch_y = batch_y.to(device)
            optimizer.zero_grad(set_to_none=True)
            loss = criterion(model(batch_x), batch_y)
            if not torch.isfinite(loss):
                raise ValueError("Non-finite training loss")
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
            loss_sum += float(loss.detach()) * len(batch_x)
            sample_count += len(batch_x)

        value = evaluate(model, x[val_mask], y[val_mask], device)
        record = {
            "epoch": epoch + 1,
            "train_loss": loss_sum / max(1, sample_count),
            "val_macro_f1": value,
        }
        history.append(record)
        print(json.dumps(record), flush=True)

        if value > best_f1 + 1e-7:
            best_f1 = value
            stale = 0
            torch.save(
                {
                    "config": config,
                    "model": model.state_dict(),
                    "validation_macro_f1": value,
                },
                run / "best.pt",
            )
        else:
            stale += 1

        if stale >= patience:
            break

    (run / "config.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    (run / "history.json").write_text(json.dumps(history, indent=2) + "\n", encoding="utf-8")
    return best_f1


def main():
    parser = argparse.ArgumentParser(description="Train the M3 causal TCN gesture model")
    parser.add_argument("--data", type=Path, required=True, help="dataset.npz")
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--epochs", type=int, default=60)
    parser.add_argument("--patience", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--hidden", type=int, default=64)
    parser.add_argument("--dropout", type=float, default=0.1)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--device", default="cpu")
    args = parser.parse_args()

    best = train(
        args.data,
        args.run,
        args.epochs,
        args.patience,
        args.batch_size,
        args.hidden,
        args.dropout,
        args.lr,
        args.seed,
        args.device,
    )
    print(json.dumps({"best_validation_macro_f1": best}, indent=2))


if __name__ == "__main__":
    main()
