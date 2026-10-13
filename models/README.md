# Models used by VirtualController M6.0

`hand_landmarker.task` is the MediaPipe tracker for 21 landmarks per hand.

A **separately trained** gesture classifier can be deployed as two matching
files (plus optional evaluation/parity report):

- `gesture_model.onnx`
- `model_metadata.json`

The app defaults to **Rules** unless Qt Creator is configured with
`VC_WITH_ONNX=ON` and a compatible ONNX Runtime SDK. With an ONNX-enabled
build, the model is loaded at tracking startup/reload only when both files
are present and pass metadata/schema/label/shape/SHA256 checks.

Input schema: `vc.hand134.v2`; float32 `[1,16,134]`, 30 Hz causal window.
Output: logits `[1,5]` in `NONE,FIST,OPEN_HAND,POINT,PINCH` order. All
normalization is inside the ONNX graph.

**This source package does not ship a trained TCN.** See `ml/README.md`
for producing one from real M5.5 JSONL recordings and for a held-out
comparison against Rules. Invalid models fall back to Rules.
