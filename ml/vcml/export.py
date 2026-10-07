from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
import torch

from .models import GestureModel
from .schema import FEATURE_COUNT, FEATURE_SCHEMA, LABELS, SAMPLE_RATE, WINDOW_SIZE


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def export_model(run: Path, output: Path):
    checkpoint = torch.load(run / "best.pt", map_location="cpu", weights_only=True)
    config = checkpoint["config"]

    model = GestureModel(config["hidden"], config["dropout"])
    model.load_state_dict(checkpoint["model"])
    model.eval()

    output.mkdir(parents=True, exist_ok=True)
    model_path = output / "gesture_model.onnx"
    sample = torch.zeros(1, WINDOW_SIZE, FEATURE_COUNT, dtype=torch.float32)

    torch.onnx.export(
        model,
        sample,
        str(model_path),
        input_names=["features"],
        output_names=["logits"],
        dynamic_axes={"features": {0: "batch"}, "logits": {0: "batch"}},
        opset_version=17,
        dynamo=False,
    )

    metadata = {
        "schemaVersion": 2,
        "modelVersion": "0.8.0",
        "featureSchema": FEATURE_SCHEMA,
        "featureCount": FEATURE_COUNT,
        "windowSize": WINDOW_SIZE,
        "sampleRate": SAMPLE_RATE,
        "labels": LABELS,
        "inputName": "features",
        "outputName": "logits",
        "normalizationEmbedded": True,
        "architecture": "causal_tcn",
        "validationMacroF1": checkpoint.get("validation_macro_f1"),
        "sha256": sha256(model_path),
    }
    (output / "model_metadata.json").write_text(
        json.dumps(metadata, indent=2) + "\n",
        encoding="utf-8",
    )

    # Optional runtime parity check when onnxruntime is installed.
    parity = {"checked": False}
    try:
        import onnxruntime as ort

        with torch.inference_mode():
            reference = model(sample).numpy()
        session = ort.InferenceSession(str(model_path), providers=["CPUExecutionProvider"])
        actual = session.run(["logits"], {"features": sample.numpy()})[0]
        max_abs = float(np.max(np.abs(reference - actual)))
        parity = {"checked": True, "max_abs": max_abs}
        if max_abs > 2e-4:
            raise ValueError(f"ONNX parity error too high: {max_abs}")
    except ImportError:
        pass

    (output / "parity_report.json").write_text(
        json.dumps(parity, indent=2) + "\n",
        encoding="utf-8",
    )
    return metadata, parity


def main():
    parser = argparse.ArgumentParser(description="Export the M3 causal TCN to ONNX")
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    metadata, parity = export_model(args.run, args.out)
    print(json.dumps({"metadata": metadata, "parity": parity}, indent=2))


if __name__ == "__main__":
    main()
