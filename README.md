# Virtual Controller - M1 hardening + immersive Controller UI (0.6.3)

Native Qt/QML application for two-hand camera tracking. This revision keeps the M1 hardening baseline and adds a deliberately small
Controller fullscreen mode. The previous 0.6.1 preview zoom/fill experiment is
not part of this build.

## Current scope

Implemented:

- Qt 6.11.2 / QML / C++20 project opened directly from the root `CMakeLists.txt`.
- Qt Multimedia camera preview and capture formats.
- MediaPipe 0.10.32 Hand Landmarker loaded in a persistent worker thread.
- simultaneous tracking of up to two hands and 21 landmarks per hand.
- explicit runtime lifecycle: `Stopped / Starting / Running / Stopping / Faulted`.
- safe `Stop -> Start`: a new session cannot start until previous MediaPipe
  teardown has logically completed.
- latest-frame mailbox (capacity 1) to prevent latency queue growth.
- temporal Left/Right stabilization across short label flips/occlusions.
- stable tracking metadata: palm position, velocity, track ID, age and last seen.
- camera FPS, processed FPS, replaced-frame rate and rolling p50/p95 latency.
- clean QML runtime state contract; mock mode remains available via `--mock-ui`.
- lightweight CTest coverage for hand identity and metrics.
- normal Controller layout remains the original M1 layout; fullscreen makes the
  camera preview the background while keeping the existing UI as translucent
  overlays.
- fullscreen-only visibility menu for camera status, gesture hints, metrics,
  hand badges and the three lower Controller cards.

Not implemented yet:

- cursor movement / One Euro / deadzone,
- mouse / keyboard output,
- gesture TCN / ONNX Runtime,
- PRESS / HOLD / RELEASE semantics,
- mapping/profile persistence,
- Xbox / PlayStation output.

## 0.6.3 UI refinement

Fullscreen now starts in a camera-focused preset: Camera status, Gesture hints,
Performance metrics and Hand state badges are ON, while Hand Tracking, Controls
and Settings panels are OFF. The preset is reapplied each time fullscreen is
entered; the `Fullscreen UI` menu can still enable individual panels or `Show all`.

The Hand Tracking card no longer duplicates Tracking FPS / latency p50 / latency
p95 because the same runtime metrics are already visible on the camera preview.

## Architecture

```text
QCamera / QVideoSink
       |
       v
LatestFrameSlot
       |
       v
MediaPipeTracker              (raw detections)
       |
       v
HandIdentityStabilizer        (stable Left / Right TrackingFrame)
       |
       +----> RuntimeMetrics
       |
       v
RuntimeController             (lifecycle + QML facade)
       |
       v
UiState.qml -> QML
```

`RuntimeController` intentionally remains the facade/orchestrator. Pure mailbox,
metrics and hand-identity logic were extracted instead of introducing a large
service hierarchy at this stage.

## Qt Creator

1. **File -> Open File or Project...** and select the root `CMakeLists.txt`.
2. Choose **Qt 6.11.2 MSVC 2022 64-bit** (compatible newer MSVC is also accepted).
3. Let Qt Creator configure CMake.
4. Build and run target `VirtualController`.
5. Open **Settings -> Camera/Tracking** if camera format or thresholds need changing.
6. Press **Start tracking** on the Controller page.

No terminal is required for normal Build / Run / Debug.

## Runtime files

The development package contains:

- `deps/mediapipe/libmediapipe.dll`
- `models/hand_landmarker.task`

CMake copies these beside the executable for Qt Creator Run/Debug and installs
them with `cmake --install` when present.

See `THIRD_PARTY_NOTICES.md` before redistribution.

## Tracking semantics

`leftConfidence` / `rightConfidence` shown by the current UI are MediaPipe
**handedness classification scores**. They are not generic hand-detection or
tracking quality values.

The stable `HandTrackingState` contains logical side, raw/effective handedness,
landmarks, palm position, velocity, track ID, age and last-seen time. Gesture or
cursor semantics are deliberately not part of this structure.

## Metrics

The runtime measures separately:

- received camera FPS,
- processed tracking FPS,
- latest-frame replacements (drop percentage),
- frame conversion/copy time,
- MediaPipe inference time,
- end-to-end processing latency from `QVideoSink` reception to tracking result.

Rolling p50/p95 values use a bounded recent-sample window.

## Tests

Core tests do not require Qt:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

See `docs/M1_HARDENING.md` for lifecycle behavior and
`docs/M1_1_IMMERSIVE_UI.md` for the fullscreen presentation change.

## Next milestone

M2 should add continuous control and cursor movement on top of `TrackingFrame`:
normalized workspace -> smoothing / One Euro -> deadzone/gain -> mock output ->
Windows mouse backend. Gesture classification remains a separate later layer.
