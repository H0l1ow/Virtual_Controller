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


def export_model(run: Path, output: Path, evaluation: Path | None = None):
    checkpoint = torch.load(run / "best.pt", map_location="cpu", weights_only=True)
    config = checkpoint["config"]

    if config.get("featureCount") != FEATURE_COUNT or config.get("windowSize") != WINDOW_SIZE or config.get("labels") != LABELS:
        raise ValueError("Checkpoint does not match the native gesture model contract")

    model = GestureModel(config["hidden"], config["dropout"])
    model.load_state_dict(checkpoint["model"])
    model.eval()

    heldout = None
    if evaluation is not None:
        report = json.loads(evaluation.read_text(encoding="utf-8"))
        if report.get("split") != "test" or report.get("datasetSha256") != config.get("datasetSha256"):
            raise ValueError("Evaluation must be for the same held-out test dataset")
        heldout = {
            "windowCount": report["windowCount"],
            "tcnMacroF1": report["models"]["TCN"]["macroF1"],
            "rulesMacroF1": report["models"]["Rules"]["macroF1"],
        }

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
        "modelVersion": "0.14.0",
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
        "heldoutEvaluation": heldout,
        "datasetSha256": config.get("datasetSha256"),
        "sha256": sha256(model_path),
    }
    # Optional runtime parity check when onnxruntime is installed.
    parity = {"checked": False, "reason": "onnxruntime Python is not installed"}
    try:
        import onnxruntime as ort

        generator = torch.Generator().manual_seed(42)
        examples = torch.cat([sample, torch.randn(5, WINDOW_SIZE, FEATURE_COUNT,
                                                  generator=generator).clamp(-5, 5)])
        with torch.inference_mode():
            reference = model(examples).numpy()
        session = ort.InferenceSession(str(model_path), providers=["CPUExecutionProvider"])
        actual = session.run(["logits"], {"features": examples.numpy()})[0]
        max_abs = float(np.max(np.abs(reference - actual)))
        parity = {"checked": True, "max_abs": max_abs}
        if max_abs > 2e-4:
            raise ValueError(f"ONNX parity error too high: {max_abs}")
    except ImportError:
        pass

    (output / "model_metadata.json").write_text(
        json.dumps(metadata, indent=2) + "\n",
        encoding="utf-8",
    )

    (output / "parity_report.json").write_text(
        json.dumps(parity, indent=2) + "\n",
        encoding="utf-8",
    )
    return metadata, parity


def main():
    parser = argparse.ArgumentParser(description="Export M6 causal TCN to ONNX")
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--evaluation", type=Path,
                        help="Held-out test JSON report; recorded in model_metadata.json")
    args = parser.parse_args()
    metadata, parity = export_model(args.run, args.out, args.evaluation)
    print(json.dumps({"metadata": metadata, "parity": parity}, indent=2))


if __name__ == "__main__":
    main()
