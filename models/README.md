# Model files

`hand_landmarker.task` is downloaded from the official MediaPipe model storage by
`scripts/bootstrap_deps.py`. It is a pretrained hand tracker, not a custom gesture model.

The application starts with geometric gesture rules. There is deliberately no fabricated
or synthetic "pretrained gesture model" in this release. Train on real recordings, evaluate,
then export `gesture_model.onnx` and `model_metadata.json` into a separate folder.
The two files must stay together. Training normalization is embedded in ONNX.
