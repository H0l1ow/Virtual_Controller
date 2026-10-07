# Virtual Controller - M3 gesture recognition (0.8.0)

Qt/QML + C++20 desktop application for two-hand camera tracking and low-latency
continuous control. M3 keeps the M1 hardening, immersive UI and M2 Windows cursor
output, then adds a separate gesture-recognition layer for both hands.

## Current scope

Implemented:

- Qt 6.11.2 / QML / C++20 project opened directly from the root `CMakeLists.txt`.
- MediaPipe 0.10.32 Hand Landmarker, two-hand tracking and stable Left/Right identity.
- explicit runtime lifecycle, latest-frame mailbox and runtime latency/drop metrics.
- M2 relative cursor control with One Euro smoothing, deadzone, sensitivity and
  DPI-like cursor speed.
- opt-in Windows `SendInput` mouse output with watchdog and F8 emergency disarm.
- **M3 static gesture recognition for both hands:** `NONE`, `FIST`, `OPEN HAND`,
  `POINT`, `PINCH`.
- causal `vc.hand134.v2` feature extractor and 16-frame / 30 Hz temporal buffer.
- deterministic geometric rule recognizer used as a safe bootstrap/fallback.
- optional native ONNX Runtime backend for a compatible causal TCN model.
- live gesture names/confidences on the Controller overlay and Gestures page.
- active recognition threshold in Settings.
- Python causal TCN training/export scaffold matching the C++ feature contract.
- pure C++ tests for gesture geometry/features/temporal recognition.

Still intentionally not implemented:

- gesture -> mouse click/scroll/keyboard actions,
- PRESS / HOLD / RELEASE event state machine, debounce/cooldown semantics,
- profile persistence / editable ActionMapper,
- dataset recording from the GUI and custom gesture creation,
- dynamic swipe gestures and two-hand gestures,
- Xbox / PlayStation output.

Those remain separate milestones so gesture classification cannot accidentally
start generating operating-system actions before its event semantics are tested.

## M3 pipeline

```text
Camera frame
    |
    v
MediaPipeTracker
    |
    v
HandIdentityStabilizer
    |
    +-------------------------------> M2 ContinuousControlInterpreter -> cursor
    |
    v
TrackingFrame (Left + Right)
    |
    v
Gesture feature pipeline
  - wrist-relative / scale-normalized landmarks
  - left-hand canonicalization
  - landmark velocity
  - finger curl
  - pinch distances
  - 30 Hz causal temporal stream
    |
    +--> rule recognizer (always available)
    |
    +--> optional causal TCN / ONNX Runtime
    |
    v
GesturePrediction { class, confidence }
    |
    v
UI only in M3
```

The gesture layer does not alter the continuous cursor path. Losing one hand
resets only that hand's gesture history. A changed `trackId` also starts a fresh
temporal window.

## Rule fallback vs TCN

A real trained TCN is **not fabricated or bundled** with this package. Without a
model, the application recognizes the initial static gestures using geometric
rules. This makes the UI/pipeline testable now while keeping the final ML
contract in place.

A compatible ONNX model uses:

```text
schema:       vc.hand134.v2
input:        float32 [batch,16,134]
sample rate:  30 Hz
output:       logits [batch,5]
labels:       NONE,FIST,OPEN_HAND,POINT,PINCH
```

Put `gesture_model.onnx` and `model_metadata.json` together in `models/`, install
ONNX Runtime 1.26.x-compatible SDK and configure:

```text
VC_WITH_ONNX=ON
VC_ONNXRUNTIME_ROOT=C:/SDK/onnxruntime-win-x64-1.26.0
```

If the model is absent or rejected, the application stays on the Rules backend
instead of breaking camera tracking.

## Current control defaults

The user-tuned M2 defaults are retained in 0.8.0:

- Mirror preview: **ON**
- Cursor speed: **2.5x**
- Invert X switch: **OFF**, while the default horizontal mapping is already
  reversed to match the mirrored interaction
- Invert Y: OFF
- Sensitivity: 70%
- Smoothing: ON
- Deadzone: 12%
- Gesture recognition threshold: 80%
- System output: OFF

## Qt Creator

1. Open the root `CMakeLists.txt` in Qt Creator.
2. Select Qt 6.11.2 MSVC 2022 64-bit (compatible newer MSVC is accepted).
3. Configure, Build and Run normally from Qt Creator.
4. Press **Start tracking**.
5. Verify cursor tracking first with `System output` OFF.
6. Open **Gestures** and verify both hands report sensible static gestures.
7. Enable `System output` only when cursor motion is already stable.

ONNX is optional. The default `VC_WITH_ONNX=OFF` build needs no ONNX SDK and uses
the rule recognizer.

## Python ML

See `ml/README.md`. The repository contains the causal TCN and ONNX export path,
but the application does not yet contain the later dataset-recording UI.

## Tests

Core C++ tests do not require Qt:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

M3 adds `vc_gesture_recognition_tests` to the existing M1/M2 test set.

Python model contract tests:

```text
python -m pytest -q ml/tests/test_model.py
```

See `docs/M1_HARDENING.md`, `docs/M1_1_IMMERSIVE_UI.md`,
`docs/M2_CONTINUOUS_CONTROL.md` and `docs/M3_GESTURE_RECOGNITION.md`.
