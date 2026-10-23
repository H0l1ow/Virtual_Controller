# VirtualController 0.16.3 — M6.4.3 Gesture Recognition Stabilization

Released: 2026-10-23  
Required base for PATCH: **0.16.2 / M6.4.2**  
Format: source files for Qt Creator, not a compiled executable.

## Scope

M6.4.2 shipped eight experimental static classes recognized via fixed landmark
thresholds. It did not contain enough confidence arbitration or tests on actual
camera recordings. M6.4.3 addresses deterministic correctness and diagnosis,
**not** the statistical lack of real-world labeled training data.

- The experimental recognizer combines normalized finger curl with landmark
  PIP/DIP joint angles. Thumb orientation is still camera-relative. This reduces
  sensitivity to some pose variations but does not make the rules a trained ML
  model. The static class catalog stays at 12 user-visible gestures.
- The independent eight-class experimental score channel is smoothed, and only
  compares scores **within itself**. These heuristic scores cannot be interpreted
  as probabilities or directly compared against ONNX output logits/posteriors.
- The winner requires a margin of at least **0.14** above the runner-up, at
  least **0.82** score (or higher global threshold) and **120 ms** of continuous
  stable observation. If a new pose is forming or ambiguous, the recognizer
  reports NONE with `POSE_STABILIZING` or `AMBIGUOUS` rather than activating a
  conflicting old `FIST`, `POINT`, `PINCH` or `OPEN_HAND` mapping.
- A strong raw new-gesture candidate suspends the legacy fallback **before**
  the filtered score reaches its recognition threshold.
- Tracking loss, identity changes, invalid geometry and capture gaps reset the
  experimental candidate. `GestureStateManager` immediately releases a held
  input upon deliberate ambiguity/stabilization; physical hand loss continues
  to release immediately as before.
- Normal event debounce/cooldown and combination priority remain in force.
  Effective onset latency includes both the new pose-stability interval and
  the event-manager debounce; this improves false-trigger resistance but
  increases delay. Thresholds were not trained/calibrated on human data.

## UI changes

The **Gestures** page now includes a responsive *Recognition diagnostics* card
using existing `VcCard`, theme, margins and font hierarchy. It shows per-hand
raw recognition, suppression reason (`AMBIGUOUS`, `POSE_STABILIZING` etc.),
experimental score margin, stable time and the top three experimental rules
scores. This adds no settings side effects and is safe to use with system
output OFF. Older Mapping editor margins/fixes from M6.4.2 are preserved.

In **ML > Record training samples**, the gesture label choices now come from
`gestureCatalog` rather than a hardcoded five-class array. `UNLABELED`, `NONE`
and all 12 user-visible static classes can be recorded in the existing JSONL v1
format. No RGB frames or audio are saved.

## Python ML compatibility contract

- `ml/vcml/schema.py` retains `LABELS = [NONE,FIST,OPEN_HAND,POINT,PINCH]` for
  M6.0 training, evaluation, exporter and ONNX metadata.
- `RECORDING_LABELS` is a separate 13-class capture-only vocabulary (`NONE`
  plus 12 gestures). Existing recordings and JSONL schema remain compatible.
- The Python JSONL reader accepts all 13 labels; the audit reports captured
  experimental counts separately and flags `EXTENDED_LABELS_RECORD_ONLY`.
- The existing five-class dataset builder *deliberately fails with a useful
  message* when extended sessions are included. It must not silently drop
  classes and misrepresent the dataset as a complete 13-class training set.
- A **versioned 13-class PyTorch/ONNX model, matched feature normalization,
  confusion-matrix evaluation, model-metadata migration and field calibration**
  are outside this update and require collected, diverse labeled data (M6.4.4).

## Validation performed

- Headless C++ CTest: **24/24**, including regression tests for all eight
  synthetic poses, both hands, two camera aspect ratios, model warmup,
  transition suppression, stale-stream reset and immediate release.
- Python app + ML pytest: **128/128**, including acceptance of extended recording
  labels and explicit rejection by the legacy dataset builder.
- The Qt Quick GUI was **not compiled or displayed on Windows** here (Qt/MSVC
  SDK unavailable). Camera tracking and real-world classification accuracy are
  **not validated**. Do not call scores probabilities or this update a trained
  recognizer.

## Manual acceptance checklist (on user's Windows machine)

1. Back up the project and user mappings, apply the PATCH to **0.16.2** or use
   FULL and rebuild with the Qt 6.11.2/MSVC 2022 x64 kit in Qt Creator.
2. With system output OFF, test each of 12 gestures separately on both hands;
   verify correct skeleton, top two scores and rejection reasons.
3. Repeat each new gesture ~20 times using varied distance, wrist rotation and
   lighting; note false recognitions and any dropped gestures. Do not map
   irreversible actions before this step.
4. Test `FIST → VICTORY`, `PINCH → OK`, `OPEN_HAND → FOUR_FINGERS` transitions;
   stale mouse/key holds should release on ambiguity. Test camera occlusion and
   reconnect, two-hand combinations, and the STOP/F8 safety path.
5. Record labeled JSONL from the ML tab; run `ml/scripts/audit_recordings.py`
   and verify extended-label counts plus explicit five-class limitation.
6. Verify translated labels and margins/scrolling at 100%, 125%, 150% DPI.

## Remaining known limitations

Geometric heuristics cannot be guaranteed accurate across anatomy, rotation,
hand self-occlusion and lighting. `THUMBS_UP/DOWN` are interpreted against the
camera vertical direction. The eight extended poses remain *Experimental*.
The next scientifically meaningful step is real-data collection and the
versioned 13-class ONNX pipeline, not further unverified threshold edits.
