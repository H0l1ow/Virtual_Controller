# M6.2.3 - Persistent System Output and Minimize Safety (v0.14.5)

## Base and installation

This update is based on **VirtualController_M6_2_2_Performance_Full.zip**,
not on stock M6.2.1, M6.2, or the original experimental ZIP. It contains
all previous M6.2.2 performance stabilizations. Do not mix this update with
the previous Compatibility Fix; use the full M6.2.3 package if your local
version is uncertain.

Install the Update ZIP over `C:\Projects\VirtualController` (extract into
`C:\Projects` with directory paths preserved) or apply the Git patch against
a clean M6.2.2 Performance Full checkout. Choose one method. Reconfigure and
Rebuild in Qt Creator using Qt 6.11.2 MSVC 2022 x64.

## Correct system-output lifecycle

Two separate states are maintained:

- **ON requested**: user opted in, remains true through transient tracking or
  camera faults and window minimization (including Windows Show Desktop).
- **Physically active**: only true while healthy, fresh tracking frames have
  passed the stable-recovery policy. A short frame/hand loss neutralizes the
  applied mouse and keyboard state without cancelling ON requested.

On recovery, a sequence of at least four consecutive tracked frames spanning
120 ms is required. This prevents blindly replaying stale inputs. Already
held mouse buttons and keys are suppressed until the gesture is first
released, then pressed again. Cursor movement does not replay old deltas.

The 250 ms OutputGuard watchdog remains in force. It intentionally releases
buttons if fresh tracking stops; this is NOT a permanent disarm. The UI
switch stays ON and shows Auto-paused until tracking is stable again.

Explicit **System output OFF**, **Stop tracking**, **F8 emergency stop**, and
**application exit** cancel the ON request. Modes that deliberately disable
system input for safe recording, calibration or offline replay retain their
existing safety boundary; they do not silently re-enable Windows input.
Arming is never restored across a fresh application launch.

## Minimize and PiP

`Main.qml` now forwards minimized/hidden window presentation state to the
runtime. This only disconnects normal page renderers, handing video to PiP if
it is enabled; it never stops the capture sink, restarts MediaPipe, resets
GestureStateManager or changes system output. The same applies to minimizing
with a gesture, via the window control, or with a Windows shell shortcut.

- Controller / ML camera previews are disabled while the main window is
  minimized or hidden. Only PiP may render the camera.
- `QMediaCaptureSession` continues to use its independent `captureSink_` for
  MediaPipe regardless of which window is visible.
- During a real driver or OS camera stall, stale physical inputs are released.
  The enabled preference stays ON and automatic camera recovery may restart
  the original camera. With automatic recovery enabled, retry does not time
  out after 30 seconds while ON was requested; it uses bounded restart delays.
- Minimizing does NOT force fullscreen on restoration, and the previous PiP
  click/double-click behavior remains unchanged.

## Test coverage

- 22 headless C++ tests, including mock input safety, watchdog resume,
  camera recovery persistence and minimized preview routing.
- 64 static Python/UI tests, including contracts for minimizing without
  calling STOP/disarm or disconnecting the native capture sink.
- 10 ML Python tests.
- The original gesture classifier, training ONNX interface, mappings,
  recordings JSONL format and desktop hotkeys are unchanged.

The test environment does NOT have Qt 6.11.2 MSVC x64, a Windows desktop or a
physical camera. Full GUI build and end-to-end gesture-minimize tests are
therefore **not verified** by these headless tests.

## Windows acceptance checklist

1. Start tracking, enable System output, verify Active, then minimize using
   the standard window button and again with the problematic gesture.
   The ON switch must remain ON and PiP should still show video.
2. With a held mouse button, briefly hide one hand; the ON preference must
   remain ON. Held OS inputs must release safely if tracking is lost. Return
   the hand and wait for stable frames: Active must return without clicking ON.
3. Minimize using Show Desktop (`Win+D`) and restore the application; verify
   that the native camera stays in the same session if it continues supplying
   frames. No second camera should be started just for PiP.
4. Cover both hands / briefly disconnect the camera. Physical inputs must
   release; reconnect the same camera and verify auto-recovery, including
   after more than 30 seconds with Auto-reconnect enabled.
5. Confirm STOP, explicit OFF and F8 *do not* auto-rearm, even if valid frames
   return. Verify that the app starts with system output OFF after restart.
6. Repeat with a 60 FPS camera and examine GUI Queue P95 / Worker Backpressure
   P95 / Capture-to-input-submit P95; test in Release before judging smoothness.

## Known safety trade-off

There is no safe way to emit real mouse/keyboard input without fresh tracking.
Thus temporary *physical* pausing is expected during actual camera stalls.
This patch prevents losing the user's ON request and restores input when
conditions permit. It does not guarantee uninterrupted active input if the
camera or entire OS stops delivering frames. F8 always takes priority.
