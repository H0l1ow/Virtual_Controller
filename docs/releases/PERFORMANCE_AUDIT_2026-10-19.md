# Virtual Controller: performance audit and experimental optimization

> **Historical baseline audit:** this file describes the experimental v0.14.3
> source before M6.2.2. The worker-to-GUI backlog and synchronous JSONL
> disk writes identified below are addressed by M6.2.2 (v0.14.4).
> See `PATCH_M6_2_2_README.md` for the current implementation and limits.

**Date:** 2026-10-19  
**Based on:** M6.2.1 / v0.14.3  
**Status:** source-level analysis and cross-platform headless tests completed;
Qt 6.11.2 / Windows / physical webcam validation **not performed**.

## Confirmed in the original source

1. The ML preview was intentionally throttled to **12 FPS** by
   `RuntimeController::recordingPreviewLimiter_{12}`; this directly explains
   visible jerkiness of the ML image even with a 30 FPS camera.
2. Each source frame also scheduled an AutoConnection lambda to the GUI thread
   to attempt preview forwarding; the 12 FPS check ran **after** GUI delivery.
   Thus a 60 FPS source could still create roughly 60 GUI callbacks/second.
   See old `RuntimeController::updatePreviewConnections()`.
3. Each processed tracking frame was posted to the GUI with
   `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`, then
   `applyTrackingFrame` generated two lists of landmark `QVariantMap`s,
   two diagnostic maps and `stateChanged()` for all state Q_PROPERTIES.
   Every input state transition and the synchronous recording append also
   happens on the GUI thread. Under load, this queue can lag. The existing
   `LatestFrameSlot` only bounds the **camera-to-worker** path, not the
   worker-to-GUI path.
4. The original worker converted `QVideoFrame` to an RGB888 QImage and
   unconditionally called `.copy()`, then copied its rows into a `std::vector`
   for MediaPipe. That extra QImage deep copy is unnecessary in this path.
5. Before this patch, the displayed end-to-end pipeline P95 was measured at
   the **end of worker processing**, before GUI queueing or action submission;
   therefore it could remain good while the user experienced late gestures.
6. The supplied ZIP only contained a prebuilt **Debug** Windows build.
   That is not evidence that the same slowdown occurs in Release.

## Changes made in this working copy

- `LatestValueMailbox<QVideoFrame>`: one thread-safe **presentation-only**
  frame slot. The camera signal publishes to it using `Qt::DirectConnection`
  with no touching of UI objects. Each new frame replaces the old one.
- `QTimer` on the GUI thread presents the latest frame, rather than queueing
  all capture callbacks to the GUI. Exactly one destination is selected by
  the existing `selectPreviewTarget` policy; switching pages and minimizing
  do **not** restart the camera or hand tracker.
- Nominal GUI presentation pacing: **Controller 33 ms (~30 FPS)**;
  **ML 50 ms (~20 FPS)**; **PiP 50 ms (~20 FPS)**. These are timer targets,
  not guarantees of achieved FPS or video/camera framerate limits.
- Full-frequency `GestureStateManager`, `GestureDataRecorder` and
  `OutputService` use the original processed-frame path. Only expensive
  presentation QVariant construction and ordinary `stateChanged` broadcasts
  are capped at **33 ms (~30 Hz)**; changes to gesture, tracking presence,
  event phase or buttons/keys trigger immediate notification.
- Gesture diagnostic maps are refreshed at most every 100 ms when steady,
  and immediately on relevant edges.
- Removed the redundant QImage `.copy()` in the worker, retaining the
  RGB888 conversion and required row copy to the MediaPipe input vector.
- Added P95 **Qt GUI queue wait** and P95 **capture-to-output-submit time**
  to Test > Performance Monitor. The latter excludes actual OS/driver input
  delivery, so it must not be misread as end-to-end screen/click latency.
- Added a Qt-independent mailbox regression test and metrics assertions.

## Headless verification

- 19 / 19 C++ CTest cases passed (`VC_BUILD_GUI=OFF`, Linux).
- 55 / 55 Python UI/static contract tests passed.
- 10 / 10 Python ML tests passed.
- Full Qt/QML executable and camera cannot be built or measured in this
  environment because Qt 6.11.2 MSVC Windows and a webcam are unavailable.

## Windows / Qt Creator validation matrix

Open the root `CMakeLists.txt` with **Qt 6.11.2 MSVC 2022 64-bit**.
Use **Release** or **RelWithDebInfo** for the final FPS comparison. Always
compare at the *same camera/format, lighting, tracking settings*.

1. Start tracking in Controller. Record Camera FPS, Track FPS, Pipeline P95,
   Qt GUI queue P95, Capture-to-input-submit P95, and CPU/GPU load.
2. Switch to ML without recording; measure the same numbers for 20+ seconds.
3. Press **Pause preview** in ML; confirm both tracking and gesture recognition
   continue and note changes in responsiveness/CPU.
4. Resume ML preview and start/stop sample recording. Compare metrics to
   identify any synchronous JSONL writer overhead.
5. Switch Controller -> ML -> Settings -> Controller repeatedly without STOP;
   verify camera/landmarks resume, no crash, and no stuck mouse/keyboard state.
6. Minimize from both Controller and ML; verify only PiP preview is rendered;
   restore using PiP. Stop and restart the camera, then quit the app normally.
7. Repeat with the 30 FPS and 60 FPS modes if supported, ideally in both Debug
   and Release builds. A large **Qt GUI queue P95** with low MediaPipe P95
   suggests event-loop congestion. A high MediaPipe P95 with high dropped
   frames suggests reducing input resolution or model cost instead.

**Safety:** Until the Windows GUI build is tested, keep system output disarmed
(`Test` / mock path), especially when evaluating gesture responsiveness.

## Rollback

Keep the original `VirtualController.zip`. Replacing the experimental files
with the originals restores the M6.2.1 pipeline. There is no model format,
profile format, dataset schema, Qt kit or CMake workflow migration in this fix.

## Remaining risks and next profiling step

- The worker-to-GUI processed-frame queue is still unbounded. Coalescing its
  messages without losing `PRESS/RELEASE`, sample recording or safety output
  requires splitting logic/output from UI presentation. Do not drop gesture
  events blindly.
- Recording JSONL is still appended synchronously on the GUI thread.
- Frame conversion remains CPU-bound; measure P95 conversion time before
  considering an RGB memory/pixel format redesign or a lower ML input size.
- `QVideoFrame` and the `VideoOutput` scene-graph resource lifetime must be
  checked with a real D3D11 Windows Qt Multimedia backend, particularly on
  tab switches and camera stop/restart.
