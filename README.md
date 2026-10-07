# Virtual Controller - M2 cursor tuning (0.7.1)

Native Qt/QML application for two-hand camera tracking and low-latency desktop
control. M2 keeps the stable M1 hardening + immersive Controller UI and adds the
first real output path: right-hand continuous movement -> logical ControllerState
-> optional Windows mouse output.

## Current scope

Implemented:

- Qt 6.11.2 / QML / C++20 project opened directly from the root `CMakeLists.txt`.
- Qt Multimedia camera preview and capture formats.
- MediaPipe 0.10.32 Hand Landmarker in a persistent worker thread.
- simultaneous tracking of up to two hands and 21 landmarks per hand.
- explicit runtime lifecycle: `Stopped / Starting / Running / Stopping / Faulted`.
- latest-frame mailbox (capacity 1), temporal Left/Right stabilization and runtime metrics.
- fullscreen Controller presentation with configurable translucent overlays.
- **M2 continuous cursor interpreter** using the stable right-hand palm position.
- **One Euro smoothing**, sensitivity/gain and radial movement deadband.
- no-jump behavior on first frame, hand reacquisition, changed track ID and long frame gaps.
- backend-neutral `ControllerState` contract prepared for later keyboard/gamepad mapping.
- opt-in Windows `SendInput` mouse backend; output is never armed automatically.
- independent output watchdog: stale tracking data disarms output and neutralizes state.
- F8 emergency output stop on Windows.
- immediate neutralization on pipeline STOP/fault and when the right hand is lost.
- mock input backend and pure C++ tests for control/output safety.

Not implemented yet:

- gesture TCN / ONNX Runtime,
- clicks/scroll/keyboard mappings driven by gestures,
- PRESS / HOLD / RELEASE semantics,
- profile persistence / real mapping storage,
- calibrated absolute workspace or configurable cursor hand,
- Xbox / PlayStation output.

## M2 behavior

M2 deliberately does **not** require a gesture to move the cursor. The stabilized
**right hand** is the continuous control source. The pipeline is:

```text
MediaPipeTracker
      |
      v
HandIdentityStabilizer
      |
      v
TrackingFrame (stable RightHand)
      |
      v
ContinuousControlInterpreter
  - One Euro filter
  - per-frame deadband
  - sensitivity/gain
  - no-jump reset rules
      |
      v
ControllerState { mouseX, mouseY }
      |                 |
      |                 +--> UI cursor preview (always safe)
      v
OutputService / OutputGuard
      |
      +--> disarmed: no OS input
      |
      +--> armed: Windows SendInput mouse backend
```

`System output` remains OFF by default. Tracking and the logical cursor preview
can therefore be tested without moving the real Windows cursor. Turning output
ON is an explicit user action.

Safety rules:

- STOP/fault -> output is disarmed immediately.
- right hand lost -> current logical output becomes neutral.
- right hand reacquired -> first frame is baseline only; no cursor jump.
- stale producer data (>250 ms) -> watchdog disarms output.
- F8 -> emergency disarm.
- a transient `SendInput` failure disarms output and reports the reason; the
  backend remains available for an explicit re-arm after the condition is gone.

## Cursor tuning

Control settings are now live C++ runtime settings rather than UI placeholders:

- **Sensitivity** - overall movement gain.
- **Smoothing** - One Euro filter on/off.
- **Deadzone** - suppresses tiny per-frame movement/jitter.

Default values remain `70 / ON / 12%`.

M2 uses relative hand movement and intentionally does not add a calibration
wizard yet. The existing disabled Calibrate button is reserved for a later
workspace/analog-control refinement.

## Qt Creator

1. **File -> Open File or Project...** and select the root `CMakeLists.txt`.
2. Choose **Qt 6.11.2 MSVC 2022 64-bit** (compatible newer MSVC is also accepted).
3. Let Qt Creator configure CMake.
4. Build and run target `VirtualController`.
5. Press **Start tracking**.
6. Move the right hand and verify the logical cursor in `Cursor Tracking`.
7. Only after that, enable **System output** to move the real Windows cursor.

No terminal is required for normal Build / Run / Debug.

## Runtime files

The development package contains:

- `deps/mediapipe/libmediapipe.dll`
- `models/hand_landmarker.task`

CMake copies these beside the executable for Qt Creator Run/Debug and installs
them with `cmake --install` when present.

See `THIRD_PARTY_NOTICES.md` before redistribution.

## Tests

Core tests do not require Qt:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Current test targets cover:

- hand identity stabilization,
- runtime metrics,
- continuous cursor/no-jump/deadzone behavior,
- output guard stale-result/one-shot semantics,
- threaded mock output STOP/watchdog behavior.

See `docs/M1_HARDENING.md`, `docs/M1_1_IMMERSIVE_UI.md` and
`docs/M2_CONTINUOUS_CONTROL.md`.

## Next milestone

M3 should introduce the gesture feature pipeline and causal TCN/ONNX inference
without changing the continuous cursor path. Gesture classification remains a
separate layer from tracking and continuous control.

## M2.1 pointer tuning

Version 0.7.1 adds live pointer tuning without changing the tracking pipeline:

- `Invert X axis` and `Invert Y axis` independently reverse cursor direction.
- `Cursor speed` is a DPI-like 0.5x-4.0x multiplier applied after smoothing and deadzone.
- `Sensitivity` remains the fine gain control; cursor speed is the coarse reach multiplier.
- The Controller quick settings expose sensitivity, cursor speed and both axis switches.
- The full Settings > Control page exposes the same controls together with smoothing and deadzone.

At 2.0x, the same physical hand displacement produces approximately twice the
cursor displacement of 1.0x, subject to the existing safety clamp.
