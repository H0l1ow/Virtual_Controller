# Virtual Controller — M6.3.3 UI Layout Refinement (0.15.3)

**M6.3.3 (from M6.3.1, not the rejected M6.3.2):** Dashboard camera status
is a translucent overlay **inside** the larger camera preview and the Dashboard
has no scrolling. Gestures now use three Settings-style sections and one outer
scroll region. In ML, recording and the session library precede model settings.
Gamepad Monitor provides two distinct Xbox/PlayStation visual layouts with
**separate** L1/L2, R1/R2 (LB/LT, RB/RT), sticks and stick-clicks.
No new gamepad driver, input action, model or fake feature was added.
Core camera/performance/persistent-arming behavior remains untouched.
See **PATCH_M6_3_3_README.md** for installation, tests and limitations.

# Virtual Controller - M6.3.1 Visual Fidelity & Responsive Layout (0.15.1)

**M6.3.1:** Reworked Dashboard, Gestures, Mapping, ML and Settings to follow
the approved five-screen visual concept while using only existing functional
C++ actions. Five primary navigation pages; Test Lab and Gamepad Monitor under
Tools; responsive gesture cards, camera-first ML recording and lazy advanced
panels. No changes to MediaPipe, model inference, persistent-arming safety or
profile/recording data formats. See **PATCH_M6_3_1_README.md** for installation,
visual acceptance checks, and the exact list of concept-only controls omitted.

# Virtual Controller — M6.3.0 UI Refresh (0.15.0)

**M6.3.0 (UI Refresh):** Collapsible navigation sidebar, simplified Dashboard,
responsive toolbar, lazy-loaded advanced cursor controls, concise Settings categories,
and an ML screen that places model status before guided recording while keeping
replay/training instructions under an expandable panel. Mapping's infrequent
Reload/Remove commands are grouped into a More menu. A few inert controls and
the unused `NavTab.qml` were removed, and historical notes moved to
`docs/releases/`. Gesture logic, camera preview routing, output safety and JSONL
format are unchanged. See **PATCH_M6_3_0_README.md**.

## Previous release: M6.2.3 Persistent Output & Minimize Safety (0.14.5)

Qt/QML + C++20 desktop application for two-hand camera tracking, gesture-driven
mouse/keyboard input, mapping profiles and an in-app Gesture Playground.


**M6.2.3 (Persistent Output / Minimize Safety):** System output ON is now a
persistent user request for the current session. Temporary hand occlusion,
missed camera frames, watchdog timeouts, camera reconnect, profile reload or
minimization/Show Desktop safely release held inputs without changing ON to
OFF. When at least one hand is tracked consistently with *fresh* data, output
resumes automatically; a held button/key must be released before it can be
pressed again. Only explicit OFF, global STOP, F8, closing the app or entering
an isolated safety/testing mode cancel the request. Preview routing now
explicitly handles minimized/hidden windows independently of capture, and
camera reconnect keeps retrying while system output was enabled. Details and
Windows acceptance checks: **docs/releases/PATCH_M6_2_3_README.md**. Output is never stored
as armed across application restarts.

**M6.2.2 stabilizes the experimental performance source (not the stock M6.2.1 ZIP).**
It keeps the presentation-only latest-frame mailbox and 30/20/20 FPS display pacing.
A worker-to-GUI in-flight gate prevents old processed results from piling up
behind the Qt event loop. No already computed gesture events are discarded;
under congestion the existing capture-side single-slot policy replaces raw
camera frames before inference, and the output watchdog remains active.
JSONL writes now run on a separate bounded file-I/O thread; Stop/fault drains
accepted samples. VideoOutput sinks release retained frames on tab/minimize
switches. Added **Worker backpressure P95** in the Performance Monitor.
See **docs/releases/PATCH_M6_2_2_README.md** for precautions and Windows checks.

**Performance audit 2026-10-19 (experimental, Windows GUI not yet validated):**
A latest-frame preview mailbox prevents capture callbacks from queuing obsolete
video frames to the GUI; render pacing is about 30 FPS Controller / 20 FPS ML
or PiP. QML state notifications are limited to about 30 Hz while tracking,
recording and input events still process every worker result. The Test tab
now measures GUI-result queue wait and capture-to-input-submission latency.
See **docs/releases/PERFORMANCE_AUDIT_2026-10-19.md** for tests, limitations and rollback.

M6.2.1 separates MediaPipe capture from the Qt Quick camera previews, routes
frames to only the currently visible Controller / ML / minimized PiP window and
suspends hidden overlays. No second camera is created. It also reduces
ML-preview rendering and avoids repeating recording geometry calculations.
See **docs/releases/PATCH_M6_2_1_README.md** for regression tests and Windows checks.

M6.2 adds guided landmark recording with an in-app camera/skeleton preview,
per-hand geometric validity progress, a session library with Open/Replay/
Recycle Bin actions and an optional user-selected recordings folder. The
existing JSONL schema and application look are unchanged. See
**PATCH_M6_2_README.md** for safety, data-quality caveats and Windows tests.

M6.1 reorganizes the existing QML navigation without redesigning the UI.
The dedicated **ML** tab provides ONNX model status/reload, landmark dataset
recording, Rules-only replay and offline Python training instructions.
**Gestures** now groups live classification, thresholds and calibration;
**Test** contains mapping tests, Cursor Playground and the Performance Monitor.
**Settings** retains app preferences, camera/PiP, tracking, cursor and desktop
integration. See **PATCH_M6_1_README.md** for all moves and test instructions.

M6.0 adds a deterministic, session/subject-safe JSONL-to-training dataset
builder, class-balanced causal TCN training, held-out TCN-vs-Rules comparison,
model/dataset SHA256 checks and deployment instructions. It does **not**
ship an unvalidated pretrained model: real labelled recordings are needed.
See **PATCH_M6_0_README.md** and **ml/README.md**.

M5.5 completes the pre-ML audit: independent camera/MediaPipe/gesture/output
metrics, Rules-only landmark replay without system output, working model reload,
per-hand movement/PINCH calibration and PiP monitor selection. Its UI scaling
remains to be validated at 100-200% Windows DPI. See **PATCH_M5_5_README.md**.

M5.4 introduces Windows System Tray integration, configurable global shortcuts,
non-destructive mapping conflict validation, and safe profile copy/delete tools.
See **PATCH_M5_4_README.md**. Builds now require the Qt Widgets module for the tray.

M5.3 introduces per-gesture recognition thresholds, an interactive left/right hand
calibration wizard (36 samples per hand/gesture), and a safe virtual Cursor
Playground with six targets, tracking metrics and live sensitivity/deadzone/
smoothing tuning. See **PATCH_M5_3_README.md**.

M5.2.2 originally added bounded recovery after reconnecting the same USB camera (the M6.2.3 persistent-output mode supersedes the old disarm behavior), three mini-preview sizes, saved preview position, optional PiP landmarks and adaptive Settings/toolbar layouts. This is based on **M5.2.1 corrected** (PiP fullscreen interaction remains reverted). See **PATCH_M5_2_2_README.md**.

M5.2 introduces QSettings-backed user settings (explicit Save/Reset),
Light/Dark/Follow Windows appearance, a mini live-camera window on minimize
(Picture-in-Picture; shares the original camera stream), and a quick profile
selector in the main top bar. See **PATCH_M5_2_README.md** for details.

M5.1 adds instant **English / Polski** UI switching in **Settings > General > Language**. The choice is persisted with `QSettings`, translations are bundled inside Qt resources, and Mapping/profile identifiers remain unchanged. See **PATCH_M5_1_README.md** for installation and language extension details.

M5.0 builds on M4.9's strict cursor gate. It adds tolerant gesture HOLD
semantics with immediate release on true tracking loss, per-hand Toggle safety,
ONNX warm-up isolation, live classifier diagnostics and labelled landmark
recording. See **PATCH_M5_0_README.md** for the full change log and install steps.

## Current gesture runtime

Active rule-recognized gestures:

- `FIST`
- `OPEN HAND`
- `POINT`
- `PINCH`

`NONE` remains an internal classifier state and is not available as a Mapping
source. The experimental 13-static / 21-total gesture registry from 0.9.6 is no
longer active. This rollback affects the gesture subsystem only; mapping,
Gesture Playground, cursor gating, output safety and the rest of M4 remain.

The optional ONNX contract remains the original five-class contract:
`NONE,FIST,OPEN_HAND,POINT,PINCH`.

## Strict cursor movement gate

Cursor movement is now denied by default.

`System output = Armed` is **not enough** to move the Windows cursor. An enabled
Mapping row with action `Enable cursor movement` must become active first.

Recommended configuration:

```text
Right + OPEN HAND -> Enable cursor movement -> Hold
```

or:

```text
Right + POINT -> Enable cursor movement -> Hold
```

With `Hold`, the cursor moves only while the mapped gesture is held. With
`Toggle`, one PRESS enables movement and the next PRESS disables it.

If no enabled `Enable cursor movement` mapping exists, cursor movement remains
locked even while output is armed.

Tracking and `ContinuousControlInterpreter` continue updating while locked, so
opening the movement gate does not create a large catch-up jump. `Freeze cursor`
remains a separate mapping action and has priority over the movement gate.

## Other implemented functionality

- MediaPipe two-hand tracking with stable Left/Right identity.
- lifecycle hardening, latest-frame mailbox and latency/drop metrics.
- relative cursor control with One Euro smoothing, deadzone, sensitivity and
  cursor-speed multiplier.
- Windows `SendInput` output, watchdog and F8 emergency disarm.
- `GestureStateManager`: PRESS / HOLD / RELEASE, debounce, cooldown and
  require-release semantics.
- `ActionMapper`: mouse buttons, wheel, cursor freeze, cursor movement gate and
  retained special/navigation keyboard actions.
- editable Mapping page with `Press`, `Hold`, `Toggle` behavior and row removal.
- JSON mapping profiles (`vc.mapping.profile.v1`), with safe duplicate/delete and live conflict diagnostics.
- Movement range and PINCH calibration for each hand; explicit Apply then Save.
- Offline Rules replay for previously recorded landmark sessions.
- Independent live performance timings for camera interval, conversion,
  MediaPipe, gesture classification, output dispatch and pipeline latency.
- Reload Model: safe worker teardown/restart; enabled output remains safely suspended until fresh tracking returns.
- PiP display selector plus size, position and optional landmark overlay.
- Windows System Tray (restore, start/stop, disarm, profile choice, exit) and global hotkeys.
- Emergency F8 remains reserved; shortcut activation never arms system output.
- Gesture Playground / Test tab with click, drag, freeze, scroll, keyboard and
  cursor-movement-gate exercises.
- Safe Internal testing through backend-neutral `ControllerState`.

## Current defaults

- Mirror preview: ON
- Cursor speed: 2.5x
- Invert X switch: OFF
- Invert Y: OFF
- Sensitivity: 70%
- Smoothing: ON
- Deadzone: 12%
- Recognition threshold: 80% (FIST/OPEN_HAND/POINT/PINCH inherit by default)
- Debounce: 120 ms
- Cooldown: 250 ms
- Require release: ON
- Gesture jitter grace: 100 ms (only while tracking is valid)
- Gesture release threshold: 55%
- System output: OFF
- Appearance: Dark (or follow Windows, or Light)
- Mini camera preview on minimize: ON
- Settings persistence: click Save (language remains automatically saved)
- Cursor movement gate: LOCKED until a mapped enable action is active

## Qt Creator

1. Open the root `CMakeLists.txt`.
2. Select Qt 6.11.2 MSVC 2022 x64.
3. Reconfigure CMake after updating.
4. Rebuild and Run.
5. Configure an `Enable cursor movement` Mapping before expecting cursor motion.
6. Test first with System output OFF in the `Test` tab.
7. Arm System output only for end-to-end Windows testing.
8. F8 remains the emergency disarm key.

The default build keeps `VC_WITH_ONNX=OFF`. To use your trained model,
install ONNX Runtime SDK, set `VC_WITH_ONNX=ON` and rebuild after export.
See `ml/README.md` for exact instructions.

## Tests

Core C++ tests:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The Qt-independent core suite contains 19 test executables. M6 adds Python
dataset/split/evaluation tests: `python -m unittest discover -s ml/tests -p 'test_*.py'`.

See `PATCH_M5_5_README.md` for the newest changes and audit of ten requested features.
See `PATCH_M5_0_README.md` for M5 installation, new gesture diagnostics,
recording and remaining limitations.
