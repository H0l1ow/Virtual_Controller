# Virtual Controller - M5.4 Desktop Integration (0.12.0)

Qt/QML + C++20 desktop application for two-hand camera tracking, gesture-driven
mouse/keyboard input, mapping profiles and an in-app Gesture Playground.

M5.4 introduces Windows System Tray integration, configurable global shortcuts,
non-destructive mapping conflict validation, and safe profile copy/delete tools.
See **PATCH_M5_4_README.md**. Builds now require the Qt Widgets module for the tray.

M5.3 introduces per-gesture recognition thresholds, an interactive left/right hand
calibration wizard (36 samples per hand/gesture), and a safe virtual Cursor
Playground with six targets, tracking metrics and live sensitivity/deadzone/
smoothing tuning. See **PATCH_M5_3_README.md**.

M5.2.2 adds bounded recovery after reconnecting the same USB camera (system input remains disarmed), three mini-preview sizes, saved preview position, optional PiP landmarks and adaptive Settings/toolbar layouts. This is based on **M5.2.1 corrected** (PiP fullscreen interaction remains reverted). See **PATCH_M5_2_2_README.md**.

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

The default build keeps `VC_WITH_ONNX=OFF`.

## Tests

Core C++ tests:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The M5.4 core suite contains 13 test executables. Additional stdlib-only Python
replay checks: `python -m unittest discover -s ml/tests -p 'test_replay_session.py'`.

See `PATCH_M5_0_README.md` for M5 installation, new gesture diagnostics,
recording and remaining limitations.
