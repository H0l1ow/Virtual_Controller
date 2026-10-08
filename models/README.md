# Model files

`hand_landmarker.task` is the pretrained MediaPipe hand tracker used to obtain
21 landmarks per hand.

M3 also supports an optional project-specific gesture model:

- `gesture_model.onnx`
- `model_metadata.json`

No fake pretrained gesture model is bundled. If these files are missing, the
application uses the geometric Rules recognizer. M4.6 recognizes 13 static
gestures with Rules and keeps the expanded catalog available independently of
ONNX.

The current ONNX contract deliberately remains `vc.hand134.v2`, float32
`[batch,16,134]` -> logits `[batch,5]`, labels
`NONE,FIST,OPEN_HAND,POINT,PINCH`. M4.6 does not fake a 22-class model: when a
legacy five-class ONNX model is active, its probabilities are merged with the
extended static Rules scores. Normalization must remain embedded in the graph.
See `ml/README.md` and `docs/M4_6_GESTURE_REFINEMENT.md`.
