# M3 gesture ML

M3 defines the gesture model contract but deliberately does **not** ship a fake
or synthetic pretrained model. Until a real `gesture_model.onnx` is available,
the C++ application uses a deterministic geometric recognizer for the static
classes `NONE`, `FIST`, `OPEN_HAND`, `POINT` and `PINCH`.

The future model consumes the same causal features as the C++ runtime:

- schema: `vc.hand134.v2`
- input: float32 `[batch, 16, 134]`
- sampling: 30 Hz causal zero-order hold
- output: logits `[batch, 5]`
- labels: `NONE, FIST, OPEN_HAND, POINT, PINCH`
- normalization: embedded in the exported ONNX graph

`dataset.npz` expected by `train.py` contains:

```text
X      float32 [N,16,134]
y      int64   [N]         # class index in the label order above
split  int8    [N]         # 0=train, 1=validation, 2=test
```

The dataset recorder/preparation UI is intentionally not part of M3. It is a
later project stage. Do not train a deployable model on synthetic landmarks and
treat the result as real gesture quality.

Training:

```powershell
python ml\scripts\train.py --data data\processed\dataset.npz --run runs\tcn_01
```

Export after installing `ml/requirements.txt`:

```powershell
python ml\scripts\export_onnx.py --run runs\tcn_01 --out models
```

Copy `gesture_model.onnx` and `model_metadata.json` together. Configure the Qt
project with `VC_WITH_ONNX=ON` and point `VC_ONNXRUNTIME_ROOT` to the ONNX Runtime
SDK. If the model is missing/incompatible, the application stays usable and
falls back to the rule recognizer.
