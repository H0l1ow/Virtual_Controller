# Virtual Controller - M1 real two-hand tracking

M1 is the first runtime-enabled iteration of the new Virtual Controller.
The existing QML interface is retained, while the Controller page is now connected
to a real Qt Multimedia camera pipeline and MediaPipe Hand Landmarker.

## M1 scope

Implemented:

- Qt 6.11.2 / QML / C++20 application opened directly from the root `CMakeLists.txt`.
- Real camera enumeration and capture format enumeration through Qt Multimedia.
- Live camera preview with `QMediaCaptureSession`, `QCamera` and QML `VideoOutput`.
- `QVideoSink` frame delivery to a C++ tracking worker.
- Single-slot latest-frame mailbox: old waiting frames are replaced instead of queued.
- MediaPipe 0.10.32 native Hand Landmarker loaded at runtime.
- Simultaneous tracking of up to two hands.
- 21 normalized landmarks per hand.
- LEFT / RIGHT handedness and confidence.
- Independent hand-loss handling: losing one hand does not stop the other hand.
- Real landmark/skeleton overlay aligned with the camera preview.
- Tracking FPS, resolution and processing latency in the UI.
- Camera, format, detection threshold, tracking threshold, mirror preview and handedness settings.
- Real runtime UI state separated from QML through `RuntimeController` + `UiState.qml`.
- Existing mock mode preserved with the `--mock-ui` command-line argument.

Not implemented in M1:

- cursor movement,
- mouse clicks / scroll,
- keyboard output,
- gesture TCN / ONNX Runtime,
- PRESS / HOLD / RELEASE gesture semantics,
- Xbox / PlayStation output.

The UI explicitly marks these areas as unavailable rather than pretending that
system output or gesture recognition is already active.

## Bundled tracking files

For this development package the existing reference project's pinned tracking
artifacts are included:

- `deps/mediapipe/libmediapipe.dll`
- `models/hand_landmarker.task`

CMake copies them next to the executable after a Windows build so Qt Creator
Build / Run / Debug does not require a separate bootstrap step.

See `THIRD_PARTY_NOTICES.md` before redistributing binaries.

## Qt Creator

1. Open the root `CMakeLists.txt` with **File -> Open File or Project...**.
2. Select **Qt 6.11.2 MSVC 2022 64-bit**.
3. Let Qt Creator configure CMake.
4. Select target `VirtualController`.
5. Build and Run normally.
6. Open **Settings -> Camera** to select a camera/format if needed.
7. On **Controller**, press **Start tracking**.

Windows camera privacy settings must allow desktop applications to use the camera.

## Expected M1 behaviour

### No hands

- LEFT: NOT TRACKED
- RIGHT: NOT TRACKED
- Tracking status: `No hands`

### One hand

- the visible hand remains tracked,
- the missing hand becomes `NOT TRACKED`,
- tracking status becomes `Degraded`,
- the pipeline continues running.

### Two hands

- LEFT and RIGHT are shown independently,
- both skeletons are rendered,
- tracking status becomes `Stable`.

### Stop tracking

- camera stops,
- landmarks disappear,
- hand confidence returns to zero,
- FPS and latency return to zero.

## Architecture

```text
QCamera
  -> QVideoSink
  -> LatestFrameSlot
  -> MediaPipeTracker worker
  -> TrackingFrame
  -> RuntimeController
  -> UiState.qml
  -> QML preview / overlay
```

The QML layer does not depend directly on MediaPipe. This keeps the runtime
replaceable/testable and leaves the existing `UiMock.qml` path available.

## Main source additions

```text
app/src/runtime/RuntimeController.hpp
app/src/runtime/RuntimeController.cpp
app/src/tracking/TrackingTypes.hpp
app/src/tracking/MediaPipeAbi.hpp
app/src/tracking/MediaPipeTracker.hpp
app/src/tracking/MediaPipeTracker.cpp
app/qml/UiState.qml
```

## Next milestone

After M1 is verified on the target Windows machine, the next implementation step
is cursor control: hand motion -> normalized workspace -> smoothing / One Euro ->
mouse movement. Gesture recognition and clicking remain separate later work.
