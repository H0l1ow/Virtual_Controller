# M6.2.1 — Camera Performance & Active Preview Lifecycle (v0.14.3)

Base: **M6.2 (v0.14.2)**. This is a regression/performance update. It does
not change the look of the app, model format, mapping profiles or JSONL schema.

## Findings

M6.2 introduced an ML `VideoOutput` beside the original Controller preview
and minimized PiP. The original Controller `VideoOutput` was ALSO the capture
session's MediaPipe input, which prevented disconnecting it safely on page
changes. Canvas skeletons in some views repainted on every landmark update.
During recording, the GUI also recomputed hand geometry already checked by
the gesture classifier and notified QML on every captured frame.

These operations can cause extra load and uneven frame delivery. They are
source-level causes/risk factors, not a confirmed on-device profiling result.

## What changed

- **One independent capture sink** (`RuntimeController::captureSink_`) is
  connected to `QMediaCaptureSession` through `setVideoSink()`. This continues
  feeding the tracking worker even when *no camera preview* is displayed.
- **Exactly one preview sink receives camera frames at a time**, prioritized:
  PiP while minimized, otherwise ML if its capture panel is visible, otherwise
  Controller if visible, otherwise none. QML switches sinks upon tab changes,
  preview pause/resume and minimize/restore. Changing tabs does NOT restart
  MediaPipe, stop sample recording, or arm/stop system input.
- **Separate rendering frame limits**, without throttling capture/MediaPipe:
  Controller: approximately 30 FPS on typical 30/60 FPS capture sources;
  ML sample preview: up to 12 forwarded frames per second;
  PiP: approximately 15-20 FPS depending on source cadence.
- **Canvas overlays only repaint while active**. The Controller skeleton
  refreshes up to ~30 times per second, ML ~10, PiP ~10. Connections to
  landmark change signals on hidden previews were removed or disabled.
- **Dataset performance**: M6.2's per-hand geometry validity is reused from
  the existing classifier result rather than recomputed on the GUI thread.
  Progress binding signals are limited to roughly 5 Hz while recording.
  The recorder format remains `vc.gestures.jsonl.v1`.
- **QML mock-mode build fix**: `readonly property var recordingQuality` is
  initialized with `({ ... })`, avoiding a QML parser error.
- Added pure C++ regression tests for frame pacing and exclusive preview
  routing. Python integration tests cover tab/minimize lifecycles.

**Important**: Only presentation work is suspended for an inactive page.
The single capture/MediaPipe worker intentionally remains running until STOP,
so input control, calibration, and ongoing dataset recording still work.

## Installation

### ZIP overlay

Unzip the `VirtualController_M6_2_1_Patch.zip` in `C:\Projects` so that
`VirtualController\...` overlays your existing M6.2 checkout. Back up local
changes first. Reconfigure and rebuild using the normal Qt Creator kit
(Qt 6.11.2, MSVC 2022 x64). No new dependencies.

### Git patch alternative

From inside `C:\Projects\VirtualController`:

```powershell
git apply --check C:\path\to\M6_2_1.patch
git apply C:\path\to\M6_2_1.patch
```

Use one update method, not both.

## Verification performed

- `VC_BUILD_GUI=OFF` CMake/C++ build: 18/18 CTest cases passed.
- UI/config/Qt contract static tests: 52/52 Python tests passed.
- ML suite: 10/10 Python tests passed.
- `git diff --check`, patch applicability, ZIP integrity and source parity
  are checked before distributing the archives.

**Not tested here**: full Qt Multimedia/QML GUI compilation on Windows,
real physical webcam, multi-monitor PiP, GPU/video renderer frame pacing,
or end-to-end measured latency. All need a Windows smoke test.

## Windows smoke tests

1. Start tracking in Controller and note frame rate + MediaPipe P50.
2. Switch Controller -> ML -> Settings -> Controller repeatedly. Video
   should resume immediately with no camera restart, worker reset or STOP.
3. In ML, start recording with one labelled hand, visit Settings briefly,
   return and Stop. Check the JSONL sequence is continuous across the tab
   switch and the file is complete. Input stays disarmed during recording.
4. Minimize from Controller and from ML. Only PiP should render. Restore by
   clicking the PiP **camera image** (normal window, not forced fullscreen).
5. From ML, click Pause preview: preview and skeleton should stop while
   recording or tracking continues. Resume to restore the preview.
6. Open the Test -> Performance Monitor. Compare before/after values for
   camera frame interval, MediaPipe and end-to-end latency. Record Windows
   CPU/GPU use, especially at 30 and 60 FPS capture formats.
7. Run `--mock-ui` to confirm the UI still parses and loads.

If the camera still stutters when all previews are hidden, investigate
tracking-thread workload / Qt camera backend separately. If it stutters
*only during actual recording*, measure JSONL synchronous disk writes; they
remain in the existing GUI recording path and were not moved to an async
writer by this low-risk patch.
