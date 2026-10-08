# Virtual Controller - M4.6 Gesture Recognition Refinement (0.9.6)

Qt/QML + C++20 desktop application for two-hand camera tracking, continuous
cursor control, gesture-driven mouse/keyboard input and an in-app mapping test
environment.

M4.6 keeps the complete M1-M4.5 pipeline and refines gesture recognition before
the project moves to gamepad work. It fixes the most visible PINCH weaknesses,
adds a single backend-owned catalog of 21 mapping gestures, and enables 13
static gestures in the deterministic Rules recognizer. The remaining 8 temporal
gestures are visible in Mapping but intentionally remain inactive until a real
temporal recognizer/model is implemented.

## Current scope

Implemented:

- Qt 6.11.2 / QML / C++20 project opened directly from the root `CMakeLists.txt`.
- MediaPipe two-hand tracking with stable Left/Right identity.
- lifecycle hardening, latest-frame mailbox and latency/drop metrics.
- M2 relative cursor control with One Euro smoothing, deadzone, sensitivity,
  2.5x default cursor speed and user-tuned mirrored interaction defaults.
- opt-in Windows `SendInput` output, watchdog and F8 emergency disarm.
- M4.6 static Rules recognition for 13 gestures: `FIST`, `OPEN HAND`, `POINT`,
  `PINCH`, `THUMB UP`, `THUMB DOWN`, `VICTORY`, `OK`, `I LOVE YOU`, `ROCK`,
  `CALL ME`, `THREE FINGERS`, `FOUR FINGERS` (plus technical `NONE`).
- optional causal TCN/ONNX backend retaining the original five-class model
  contract, merged with the extended static Rules recognizer.
- **M4 GestureStateManager:** PRESS / HOLD / RELEASE, debounce, cooldown and
  require-release semantics.
- **M4 ActionMapper:** gesture events can drive mouse buttons, wheel and keyboard.
- Windows keyboard injection for the currently retained special/navigation keys.
- editable Mapping page with `Press`, `Hold`, `Toggle` behavior.
- JSON mapping profiles in schema `vc.mapping.profile.v1`, saved atomically.
- Default / Desktop / Presentation / Game seed profiles.
- live event state (`PRESS`, `HOLD`, `RELEASE`, `IDLE`) on the Gestures page.
- **M4.5 Gesture Playground / Test tab** with mapping-aware click targets, right-click
  targets, drag-and-drop, freeze/reposition, scroll and key-action exercises.
- Safe Internal test mode observes ControllerState after ActionMapper while Windows
  output remains disarmed; System output can be enabled for a full SendInput check.
- `Run all` walks every enabled testable mapping (and both click + drag exercises for
  left-click Hold mappings), with pass/fail counters and live diagnostics.

Still intentionally not implemented:

- temporal execution for `SWIPE LEFT/RIGHT/UP/DOWN`, `CIRCLE CW/CCW`, `PUSH`,
  `PULL` (IDs are already present in Mapping but can only be saved disabled),
- two-hand relationship controls,
- Xbox / PlayStation virtual gamepad output,
- arbitrary key-combination/chord editor,
- GUI dataset recording and custom gesture training.

## M4 pipeline

```text
Camera
  -> MediaPipeTracker
  -> HandIdentityStabilizer
  -> TrackingFrame
       |
       +-> ContinuousControlInterpreter -> cursor delta ---------+
       |                                                         |
       +-> GestureEngine -> GesturePrediction                    |
                              |                                  |
                              v                                  |
                       GestureStateManager                       |
                       PRESS/HOLD/RELEASE                        |
                              |                                  |
                              v                                  |
                         ActionMapper                            |
                       mouse / keyboard                          |
                              |                                  |
                              +---------------+------------------+
                                              v
                                      ControllerState
                                              |
                                      OutputGuard / F8
                                              |
                                      Windows SendInput
```

Gesture recognition still does not know about Windows. Mapping still does not
know whether M3 used Rules or ONNX. Continuous cursor control still does not
require a gesture.

## Default desktop behavior

The shipped `Default` profile enables:

- Right `PINCH` -> left mouse button, `Hold` behavior.

This means pinch can be used for click-and-drag while the right hand continues
to move the M2 cursor. Other example rows are shipped disabled or are enabled
only in explicitly selected profiles.

System output remains OFF after application start. Enable it only after cursor
and gesture detection are behaving correctly. F8 immediately disarms output.

## M4.6 gesture catalog and PINCH refinement

Mapping no longer owns a hard-coded four-item gesture list. The runtime exposes
one 21-gesture catalog used by Mapping and the Gestures library.

Available now (Rules):

```text
Fist, Open Hand, Point, Pinch, Thumb Up, Thumb Down, Victory, OK,
I Love You, Rock, Call Me, Three Fingers, Four Fingers
```

Reserved temporal IDs:

```text
Swipe Left, Swipe Right, Swipe Up, Swipe Down,
Circle CW, Circle CCW, Push, Pull
```

Temporal gestures are shown in Mapping so profile design is stable, but they
cannot be enabled yet. This prevents the UI from pretending an unimplemented
gesture can emit an action.

PINCH now uses a rule-specific distance that greatly reduces the contribution
of noisy landmark Z, has priority over FIST when thumb/index contact is clear,
and uses a Schmitt-style enter/release latch to reduce PINCH/NONE flicker while
the fingertips remain near the contact boundary. Rule score smoothing now
reacts faster; PRESS/HOLD/RELEASE debounce remains a separate M4 layer.

## Gesture settings

`Settings -> Gestures` is active in M4:

- Recognition threshold: classification confidence threshold.
- Debounce: how long a gesture must remain stable before PRESS.
- Cooldown: quiet interval after RELEASE before another PRESS.
- Require release: after RELEASE, require one complete observation away from the
  released gesture before a fresh candidate can debounce.

Changing event-state settings while output is armed disarms output for safety.

## Mapping profiles

The repository contains example JSON files in `profiles/`. On first run they are
seeded into the writable Qt application config directory. The Mapping page shows
the exact writable directory.

A mapping stores:

```text
hand + gesture + action + behavior + enabled
```

Supported actions currently include mouse left/right, wheel up/down, cursor freeze,
Space, Enter, Escape, Tab, arrow keys and Ctrl/Shift/Alt. Letter-key mapping remains
intentionally deferred until the keyboard-control design is defined.

## Current control defaults

User-tuned M2 defaults are retained:

- Mirror preview: ON
- Cursor speed: 2.5x
- Invert X switch: OFF; default horizontal mapping is already reversed for the
  mirrored interaction
- Invert Y: OFF
- Sensitivity: 70%
- Smoothing: ON
- Deadzone: 12%
- Gesture recognition threshold: 80%
- Debounce: 120 ms
- Cooldown: 250 ms
- Require release: ON
- System output: OFF

## Qt Creator

1. Open the root `CMakeLists.txt`.
2. Select Qt 6.11.2 MSVC 2022 x64 (compatible newer MSVC is accepted).
3. Reconfigure CMake after updating from M4.5 / 0.9.5.
4. Build and Run.
5. Start tracking with System output OFF.
6. Verify gesture event states on the Gestures page.
7. Check/edit mappings on the Mapping page.
8. Open **Test** and validate mappings in Safe Internal mode first.
9. Optionally enable System output on the Test page for an end-to-end Windows check.
10. Use F8 as emergency disarm.

The default build keeps `VC_WITH_ONNX=OFF`; Rules remains available without an
ONNX SDK/model.

## Tests

Core C++ tests:

```text
cmake -S . -B build-core -DVC_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The current core suite contains 8 test executables. The gesture-recognition
suite now covers all 13 active static gestures, the 21-entry registry, PINCH vs
FIST ambiguity, depth-noise tolerance, hysteresis, temporal feature buffering,
left/right canonicalization and engine reset behavior.

See `docs/M4_6_GESTURE_REFINEMENT.md` for the exact M4.6 recognition and
mapping-catalog contract.


## M4.1 mapping refinements

- `Freeze cursor` is a normal mapping action (`control.cursor.freeze`) and can be assigned to any supported gesture/hand.
- `Hold` freezes only cursor motion while hand tracking keeps updating, so releasing the gesture does not create a large catch-up jump.
- `Toggle` can be used for a persistent freeze/unfreeze workflow.
- Mapping rows can be removed from the always-visible `Remove` button in the Mapping toolbar as well as the details panel.
- `Freeze cursor` is available only as a selectable Mapping action; no gesture is assigned to it by default.
## M4.2 mapping corrections

- `Freeze cursor` remains a normal Mapping action, but it is no longer pre-assigned to `FIST` in the bundled Default profile.
- Letter-key actions (`Key A` through `Key Z`) were removed from the mapping UI and C++ logical action/backend layer. Keyboard interaction is intentionally limited to the remaining special/navigation keys until a proper keyboard-control design is defined.
- The accidental 0.9.1 seeded `Right + FIST -> Freeze cursor` row is ignored on load by its legacy seed ID. User-created freeze mappings (`custom_*`) remain fully supported.



## M4.5 Gesture Playground

The new `Test` tab is intentionally mapping-driven: it never hard-codes PINCH, FIST
or another gesture to an exercise. It reads the active enabled JSON mappings and
builds appropriate exercises from their logical actions.

Available exercise types:

- left-click targets,
- right-click targets,
- drag-and-drop for left mouse `Hold`,
- cursor freeze/reposition/unfreeze with release-jump observation,
- scroll direction/step validation,
- current special/navigation key actions.

Safe Internal mode uses the same `GestureStateManager -> ActionMapper ->
ControllerState` path as normal control, but can be tested with System output OFF.
The logical cursor is drawn inside the exercise board using the same cursor state as
the Controller page.
