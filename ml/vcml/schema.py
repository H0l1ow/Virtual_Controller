from __future__ import annotations

FEATURE_SCHEMA = "vc.hand134.v2"
FEATURE_COUNT = 134
WINDOW_SIZE = 16
SAMPLE_RATE = 30
MAX_GAP_US = 150_000
LABELS = ["NONE", "FIST", "OPEN_HAND", "POINT", "PINCH"]
# Accepted labels in the recording/audit pipeline. LABELS remains the immutable
# five-class M6.0 ONNX contract; a versioned 13-class trainer will be separate.
RECORDING_LABELS = LABELS + [
    "THUMBS_UP", "THUMBS_DOWN", "VICTORY", "ROCK", "THREE_FINGERS",
    "FOUR_FINGERS", "CALL_ME", "OK",
]
