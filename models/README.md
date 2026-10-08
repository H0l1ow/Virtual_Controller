# Model files

`hand_landmarker.task` is the pretrained MediaPipe hand tracker used to obtain
21 landmarks per hand.

M3 also supports an optional project-specific gesture model:

- `gesture_model.onnx`
- `model_metadata.json`

No fake pretrained gesture model is bundled. If these files are missing, the
application uses the static geometric Rules recognizer and remains fully usable
for M3 testing.

The ONNX contract is `vc.hand134.v2`, float32 `[batch,16,134]` -> logits
`[batch,5]`, labels `NONE,FIST,OPEN_HAND,POINT,PINCH`. Normalization must be
embedded in the graph. See `ml/README.md` and `docs/M3_GESTURE_RECOGNITION.md`.
