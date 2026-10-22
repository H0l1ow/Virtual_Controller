# M6.2.2 — Performance Stabilization (v0.14.4)

## Important: which version to update

**Base:** `VirtualController_Performance_Experimental_Source(1).zip`, dated
2026-10-19. This patch is **not** intended for stock M6.2.1 or M6.2.0.
It preserves the experimental preview mailbox, exclusive active preview,
30/20/20 FPS nominal display pacing, reduced UI binding churn, and the
conversion change that removed `QImage::copy()`.

This release modifies neither gesture classes/threshold algorithms, model
contract, QML visual design, profile JSON nor the `vc.gestures.jsonl.v1`
recording schema.

## Changes

1. **Bounded worker-to-GUI result delivery** (`GuiResultBackpressure.hpp`):
   a single in-flight processed tracking result is permitted. The worker
   waits **before processing the next camera frame** if Qt hasn't finished
   consuming the previous one. The capture mailbox already keeps the latest
   camera frame; older *unprocessed* frames can be replaced under overload.
   No already computed PRESS/HOLD/RELEASE is dropped or reordered. A queued
   Qt callback retains a lease until execution or disposal. Waiting is
   interruptible by session close and stop token, preventing a STOP deadlock.
2. **Input safety:** existing immediate STOP/fault disarm and 250 ms stale
   data watchdog are preserved. Backpressure is never applied after
   `GestureStateManager` or `ActionMapper`, so generated events are not
   discarded. Sustained GUI stalls may still result in frame skips or watchdog
   disarm; this is safer than replaying stale clicks later.
3. **Asynchronous JSONL disk writing**: `GestureDataRecorder` serializes
   frame data on its caller thread but queues completed lines to a separate
   writer thread. The queue has an upper bound of 256 lines. Queue saturation
   stops recording with an explicit error; it does **not** silently drop
   accepted samples. A normal Stop drains the queue, flushes and closes the
   file before marking the session complete. Camera stop, fault and reconnect
   also stop recording cleanly. The file format is unchanged.
4. **Qt video resource cleanup:** invalidate old preview frames on page,
   PiP, preview pause and application shutdown. This avoids retaining the
   last QVideoFrame and potential GPU resources of inactive views. A single
   independent capture sink continues feeding MediaPipe as before.
5. **Observability:** `Test -> Performance Monitor` now includes
   **Worker backpressure P95** — time spent waiting before inference for a
   free GUI-result slot. Read it together with Qt GUI Queue P95 and the
   existing capture-to-input-submit P95. Backpressure can reduce achieved
   Track FPS while keeping input fresh. There is no change to the actual
   output-input rate limiter or its gesture state machine.

## Performance tradeoffs

- Processing may skip more *raw* frames during a congested GUI period. That
  can reduce dataset throughput and could miss extremely brief gestures.
  Already-classified events never get dropped in a result queue. The existing
  250 ms input watchdog safely releases buttons if tracking becomes stale.
- The JSONL serializer itself remains on Qt's runtime callback; the slow
  filesystem write is on the writer thread. File close drains at most the
  bounded queue, so exceptionally slow/unresponsive storage can still make
  Stop take longer. The operation is not completely asynchronous through to
  the Stop button.
- The worker must continue to run in the background while other cards are
  visible, to retain gesture control, calibrations and active recording.
  Only unused VIDEO PREVIEWS are detached/cleared.

## Verification completed in the build environment

- C++/CMake headless core: 21 CTest targets, including new multithreaded
  backpressure/shutdown and asynchronous JSONL flush tests.
- Python UI/static regression contracts and ML tests run without a webcam.
- Verified the patch diff is generated from the *exact* provided experimental
  source ZIP and recreates the full updated source tree.

**Not verified:** Qt 6.11.2 + MSVC 2022 x64 full GUI compilation and D3D11
frame lifetime on a real Windows PC. Do not treat C++ core / source tests as
proof that the Windows camera FPS or latency has improved.

## Install and rollback

1. Save your current project as a backup, especially uncommitted local work.
2. Overlay the supplied **M6.2.2 Update ZIP** onto the extracted
   `VirtualController_Performance_Experimental_Source(1).zip` structure; or
   apply the `M6_2_2_Performance_Stabilization.patch` with Git. **Not both.**
3. In Qt Creator use **Qt 6.11.2 / MSVC 2022 x64**, Reconfigure and Rebuild.
4. If problems appear, use the original experimental ZIP for rollback.

## Windows acceptance tests (output disarmed first)

1. Start Camera in Controller; capture media FPS, Track FPS, GUI Queue P95,
   Worker backpressure P95, Capture-to-input-submit P95 and CPU usage in
   **Release**; compare the same camera format against experimental v0.14.3.
2. Switch Controller -> ML -> Settings -> Controller, then minimize to PiP;
   ensure only one VideoOutput renders, the camera doesn't restart, and no
   freeze/black screen persists when reactivating the preview.
3. Begin a labelled JSONL recording, switch away from ML, return and Stop.
   Check that the JSONL is readable, its line count equals reported accepted
   frames, and Replay can open it. With a normal SSD there should be no
   large UI stalls during recording.
4. While recording, stop or disconnect the camera. Confirm the file closes,
   input disarms, and the session can be replayed without corruption.
5. Test a sustained PINCH drag and rapid PRESS/RELEASE changes with system
   output still disarmed; check in-app logical state and event transitions.
   Only then arm physical output for manual tests. If the GUI is overloaded,
   the watchdog must fail closed instead of leaving a button held.
6. Test 30 and 60 FPS camera modes. Compare FPS and the P95 latencies, not
   just how smooth the VideoOutput looks. If the backpressure P95 becomes high,
   investigate QML or synchronous frame serialization work.

## Files added

- `app/src/runtime/GuiResultBackpressure.hpp`
- `app/src/gestures/AsyncJsonlWriter.hpp/.cpp`
- `tests/GuiResultBackpressureTests.cpp`
- `tests/AsyncJsonlWriterTests.cpp`
- `tests/test_perf_stabilization.py`

No additional external dependencies are required.
