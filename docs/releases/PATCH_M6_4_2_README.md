# VirtualController 0.16.2 — M6.4.2 Static Gesture Recognition

Release date: 2026-10-23  
Starting version: 0.16.1 / M6.4.1  
Patch style: source-code update; no installer or EXE included.

## Delivered

### Eight additional experimental static gestures

- `THUMBS_UP` — four fingers curled, thumb pointing vertically up.
- `THUMBS_DOWN` — four fingers curled, thumb pointing vertically down.
- `VICTORY` — index and middle extended/separated, ring and pinky curled, thumb folded.
- `ROCK` — index and pinky extended, middle and ring curled, thumb folded.
- `THREE_FINGERS` — index/middle/ring extended, pinky curled, thumb folded.
- `FOUR_FINGERS` — four fingers extended, thumb folded.
- `CALL_ME` — thumb and pinky extended, other fingers curled.
- `OK` — bent index and thumb make a ring, other three fingers extended.

These are *functional* landmark-rule recognizers in C++ and are eligible for
single-hand and ordered left/right combination mappings. Existing classes
`FIST`, `OPEN_HAND`, `POINT`, `PINCH` remain unchanged. Thus there are twelve
currently mappable static poses, giving up to `12 x 12 = 144` ordered left/right
combinations. The dynamic/temporal catalog entries remain planned.

All eight new recognizers are labelled **Experimental**, not proven reliable.
Their synthetic fixtures passing unit tests does not demonstrate high accuracy
in uncontrolled camera conditions. Users must test lighting, hand shapes,
orientation, camera aspect ratio, occlusion, fast motions and false activations.
The thumb-up/down rule interprets *camera-image vertical*, not a world-aligned
3D orientation. An `OK` pose explicitly requires the index to bend, to avoid
confusing an existing index/thumb `PINCH` with an `OK` sign.

### Architectural and safety decisions

- Preserve ONNX output **five classes** and feature schema `vc.hand134.v2`.
  Legacy 5-class ONNX output and the original per-class calibration indices are
  unchanged. No model retraining is required to test the eight landmark rules.
- Keep the expanded `GestureClass` values and the canonical registry in the
  mapping/profile layer, separate from five-class ONNX output arrays.
- Smooth experimental pose scores and use existing debounce, cooldown and
  release hysteresis. On physical loss of a hand, output is released immediately.
- The original four gestures retain their per-class thresholds and calibration
  workflow. For the eight experimental poses, the current global recognition
  threshold is used with a conservative 80% floor; individual tuning/training
  is a later task.
- `GestureStateManager`, diagnostics and calibration APIs guard against array
  indices beyond the five legacy scores. The diagnostics expose the new scores
  separately without altering ONNX or Python training tensors.
- JSON profile schema remains `vc.mapping.profile.v2`; existing v1 migration
  and user profile backups remain available. New gesture IDs are serialized
  by canonical name and are recognized by v2. No default mappings were changed.
- Source input backends and gesture-combination arbitration remain unchanged.
  Output service should be kept disarmed while validating experimental poses.

### Mapping UI correction based on the supplied screenshot

- Fixed the right-side `Edit mapping` Popup: `anchors.margins` was set on a
  non-anchored `ColumnLayout`, making margins ineffective. The scroll content
  now has explicit 20-pixel left/right insets, with matching header and footer
  insets, and a horizontal-scroll guard.
- Pair input fields continue to use two columns when space allows; timing
  fields also collapse to one column in a narrow editor.
- The left gesture filter builds from the 12 available static poses and has
  its own clipped vertical list, avoiding overlap with adjacent cards.
- ComboBox dropdowns now have capped height and internal scrolling, necessary
  when selecting among 12 gestures.
- Gesture names in the editor use localized catalog titles instead of raw
  uppercase tokens. Experimental selection is marked with a caution note.
- The Gestures page shows **Available / Experimental / Planned** as separate
  statuses. Existing navigation, theme colors, cards, text hierarchy and
  profile actions are preserved.

### Acceptance tests available here

- Native headless CTest: **24 of 24 passing**. One added target tests all eight
  new poses, both hands, two camera aspect ratios, ONNX warmup, press/hold/
  release and no false rule collision with the four legacy synthetic poses.
- Python pytest: **123 of 123 passing**, including architecture, UI margin,
  profile and previous-stage regression contracts.
- GUI build, Qt Quick rendering on Windows, MediaPipe runtime, actual webcam
  recognition and user-specific accuracy were **not** verified in this
  environment. They must be checked using Qt Creator and the user's camera.

## Upgrade from 0.16.1

1. Back up the project and locally saved profiles.
2. Extract PATCH into the parent folder of `VirtualController` (it contains a
   `VirtualController/` prefix). Alternatively extract FULL as a fresh folder.
3. Open root `CMakeLists.txt` in Qt Creator with Qt 6.11.2 / MSVC 2022 x64.
4. Reconfigure CMake and rebuild; start the application.
5. With output disarmed, visit Gestures to test each new pose and then Mapping
   to assign singles/combinations. Save the profile explicitly.

### Known limitations / future work

- User-specific real-camera recordings and confusion matrix are still missing;
  there is no validated ONNX model trained on the expanded gesture set.
- Experimental poses have fixed geometric templates; users with different
  finger mobility, camera angles or perspective may see misses.
- Motion, time-based gesture events and physical two-hand spatial relations
  remain M6.5/M6.6, with virtual gamepad output remaining M7.0.
- Older versions may not understand profiles containing M6.4.2 gesture IDs.
  Retain profile backups and do not open such profiles in 0.16.1 expecting
  full roundtrip compatibility.
