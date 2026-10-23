# VirtualController — M6.4.1 Gesture Engine Expansion

**Status:** initial design baseline for the implemented 0.16.1 update. Final decisions and limitations are documented in PATCH_M6_4_1_README.md.

**Analyzed package:** `VirtualController.zip`, app `0.16.0`, stage `M6.4.0`. Read with `Virtual_Controller_Zalozenia_Projektowe.pdf` and supplied UI references.

## 1. Verified baseline

- Runtime is C++20, Qt 6.11.2 / QML, MediaPipe tracking two hands, rules recognizer with optional ONNX runtime, mapping via ActionMapper, and Input/OutputService.
- Available runtime gestures: `FIST`, `OPEN_HAND`, `POINT`, `PINCH`; internal `NONE`. The class count and order (5 total including NONE) are fixed in `GestureTypes.hpp`, ONNX metadata validation and `ml/vcml/schema.py`.
- `GestureStateManager` emits one `Press/Hold/Release` event per hand per frame, with hand tracking status and `trackId`; loss/change of tracked identity releases immediately.
- `ActionMapper` processes independent L/R events. It supports `Press`, `Hold`, `Toggle`, handles cursor movement gate and hand-loss neutralization, and has no combination trigger type.
- `ProfileStore` reads and writes exclusively `vc.mapping.profile.v1`, stores user profiles in `QStandardPaths::AppConfigLocation/profiles`, and seeds bundled profiles only when missing. `QSaveFile` is already used for atomic writes.
- `MappingValidator` diagnoses duplicate IDs, conflicting behaviors, cursor gating/freeze and certain drag conflicts, but knows only individual gestures.
- QML Mapping uses a two-column responsive `VcCard` layout, gesture filters, assigned-action list, and a right-side modal editing popup. Gestures has 3 grouped sections: thresholds, calibration, advanced timing. Palette/layout are centralized in `Theme.qml`.
- Gamepad page is a layout/preview, not a connected output backend (`VC_WITH_GAMEPAD=OFF`). `VC_WITH_ONNX=OFF` is the current default in ZIP even though the earlier design PDF proposed ON.
- Packaged recording audit reports zero recordings and no trained ONNX gesture model is present.

**Baseline validation done:** 22/22 CTest native tests and 115/115 pytest tests passed (headless Linux, GUI disabled). Windows/MSVC GUI build and live camera still require verification in Qt Creator.

## 2. Scope and exclusions

### Implement in M6.4.1

1. Metadata-driven, backend-neutral gesture catalog with category (`Static`, `Dynamic`, `Temporal`, optionally `Continuous` as capabilities), canonical ID, localized display label, availability (`Available`, `Experimental`, `Planned`), and recognized-by capability.
2. Separate combination evaluator recognizing ordered `LeftGesture + RightGesture`, e.g. `FIST + FIST`.
3. Configurable pair synchronization window and simultaneous hold duration, independent from existing individual debounce/cooldown settings.
4. Explicit conflict arbitration: valid combination takes precedence over conflicting individual gestures and fires one mapped action. All involved singleton `Press`, `Hold`, `Toggle` actions must be delayed/suppressed appropriately **before** dispatch.
5. Immediate combo release on loss of either tracked hand or identity change; output neutralization during STOP/fault/reload/profile changes. No stuck inputs.
6. Per-profile combination and single-hand mapping storage with compatible `v1 -> v2` import.
7. Minimal integrated QML changes and PL/EN localization; no new top-level navigation page.
8. Regression and unit tests.

### Not in M6.4.1

- Claiming 12 recognized static classes; those require implementation and validation in M6.4.2.
- Dynamic gesture recognition or training a new temporal ONNX model; M6.5.
- Two-hand spatial interaction (relative distance/angle) and sequences/continuous controls; M6.6.
- Real Xbox/PlayStation input driver and mapping to working pad buttons; M7.0.
- New screenshot, media-control, key-chord or profile-change actions **unless specifically scoped as an action backend extension**; the current `LogicalAction` enum does not implement all example actions, e.g. `Ctrl+C`, screenshot, native media play/pause or real pad buttons.

## 3. Recommended architecture

Existing:

```
TrackingFrame
  -> GestureEngine (L/R recognized separately)
  -> GestureStateManager (per-hand PRESS / HOLD / RELEASE)
  -> ActionMapper (independent gestures)
  -> ControllerState
  -> OutputService
```

Proposed:

```
TrackingFrame
  -> GestureEngine (keep ONNX five-class contract untouched in M6.4.1)
  -> GestureStateManager (unchanged per-hand lifecycle)
  -> TriggerCombinationEngine / GestureArbiter
       - per-hand active state and track ID
       - pair candidates, synchronization window, overlap hold
       - suppress/defer competing single mappings
       - output normalized TriggerEventFrame
  -> ActionMapper (single / two-hand triggers; existing actions)
  -> ControllerState
  -> OutputService
```

Keep pair evaluation deterministic, clocked by monotonic `timestampUs` from camera processing, and independent of QML frame rate. Observe all worker frames; UI updates remain paced separately. Reinitialize on backward timestamp, model reload, profile change, STOP, tracking reset.

Do **not** use 144 new ML classes: the combinations are a product of two independently recognized classes. At present only 4x4 = 16 possible ordered pairs are technically recognizable; 12x12 = 144 only after new static gestures become available.

Prefer a `GestureRegistry` with stable ID strings separate from `GestureClass` classifier labels; `OPEN_HAND` is the persisted canonical ID, while `OPEN HAND` is an existing UI display string. Avoid changing the ONNX label order or feature schema in M6.4.1.

## 4. Trigger and event data model

- `TriggerKind`: `Single`, `TwoHandCombination`, reserved `Sequence` / `TwoHandSpatial`.
- `HandSelector` for singles: `Left`, `Right`, `Any`.
- `GestureId`: stable canonical ID defined by the shared catalog. Unknown/planned ID must not generate events.
- Pair spec: `{ leftGesture, rightGesture, synchronizationMs, overlapHoldMs }`.
- `TriggerEvent`: `{ mappingId/triggerId, phase, timestampUs, participating trackIds, confidence }`.
- Retain `ActionBehavior` and `LogicalAction` compatibility; plan an independent action registry for more outputs later.
- A rule with `hand=Any` should emit at most one action if both hands show the same class; maintain an owning hand/track ID until a release to avoid duplicate toggles.

Suggested defaults **to validate**, not inherited from current runtime: synchronization 250 ms, simultaneous overlap 150 ms. Allow override per combo, and optionally profile-level defaults. Keep these settings distinct from per-hand `debounceMs`, `cooldownMs` and `dropoutGraceMs`.

### Exact arbitration policy

1. Only enabled pairs with two recognized, tracked hands are candidates.
2. If any enabled pair might match, defer conflicting single `Press` and `Toggle` edges and single `Hold` activation during the configured pair window.
3. Once the actual pair stays simultaneously valid for its hold duration and synchronization criterion is met, emit exactly one combination `PRESS` and suppress the involved individual actions. Continue `HOLD` while valid.
4. On one-hand loss, changed `trackId`, pair breakup, STOP, profile/model reload, emit a single combination `RELEASE` / neutralization immediately (do not apply dropout grace to physical loss).
5. If the combination does not materialize in the window, allow unaffected singles to proceed with correct buffered-edge semantics. A held singleton should not be lost simply because a pair was theoretically possible.
6. A previously activated pair must require a new valid activation edge before it can fire again. Releasing one hand should not cause suppressed individual actions to appear spontaneously; require a fresh qualifying individual activation.
7. Without an enabled pair for the candidate gestures, preserve original single-gesture latency and semantics.
8. Reject or flag duplicate enabled pair bindings and invalid timing/unsupported gestures. Do not hide conflicts.
9. Do not perform key or mouse output directly from the pair engine. Output stays behind `ActionMapper` + `ControllerState` + `OutputService`.

**Key acceptance case:** `Left=FIST, Right=FIST` with enabled pair and enabled individual FIST mappings => one pair `PRESS`, one continuous pair `HOLD`, one pair `RELEASE`, zero individual actions during its activation.

## 5. Profiles and migration

Extend `vc.mapping.profile.v1` to `vc.mapping.profile.v2` without overwriting the user profile folder. On v1 read, translate every legacy mapping into a `Single` trigger in memory with the same `id`, gesture, side, output/action, behavior and enabled flag. Preserve order. Use `QSaveFile` to atomically write v2 on explicit save, preferably with a backup/rollback path. Existing seeded profiles must not overwrite user copies.

Proposed **illustrative** v2 record (final schema can be adjusted before coding):

```json
{
  "schema": "vc.mapping.profile.v2",
  "name": "Desktop",
  "mappings": [
    {
      "id": "right_pinch_left_mouse",
      "trigger": { "type": "single", "hand": "Right", "gesture": "PINCH" },
      "action": "mouse.left", "behavior": "Hold", "enabled": true
    },
    {
      "id": "both_fist_space",
      "trigger": { "type": "two_hand_combo", "left": "FIST", "right": "FIST",
                   "synchronizationMs": 250, "overlapHoldMs": 150 },
      "action": "key.space", "behavior": "Press", "enabled": true
    }
  ]
}
```

`key.space` is currently implemented. Screenshot/media/ctrl-chord/pad actions shown in user scenarios are examples of future output extensions, not proof of existing support.

Use strict field validation for v2: invalid or unknown fields should cause explicit errors rather than silently deleting mappings during read-save roundtrip. Preserve disabled planned mappings for later activation where possible; never dispatch their actions.

## 6. QML integration with the current UI

- **MappingPage.qml:** retain the existing top profile toolbar, two-column `VcCard` composition, `Mapping validation` disclosure and right-side editing popup. Add a compact selection at the top of the popup: `Single gesture` / `Two-hand combination` / `Gesture sequence (planned, disabled)`.
- **Single mode:** keep hand selector and gesture/action/behavior controls; extend hand selector to `Any hand`.
- **Pair mode:** show `Left gesture` and `Right gesture` side by side on wide popup, stacked on narrow; reuse `GestureGlyph` and `Theme.leftHand` / `Theme.rightHand`, add optional advanced disclosure with timing fields. Only show selectable recognized classes.
- **Assigned actions:** retain the current list/card style; print `L: FIST + R: PINCH`, action, behavior, and enabled status. Ensure the filter can show singles and combinations without assuming a `source` string equals one gesture.
- **Gesture filter:** with 20+ planned classes, replace the hardcoded tall filter cards with compact search/category/list or an adaptive scrollable area, preserving the same visual components. Avoid expanding the fixed-height card to fit all gestures.
- **GesturesPage.qml:** preserve the three main tabs and the existing four active threshold sliders. Add a compact catalog/view of supported vs experimental/planned classes inside the Gestures section. Planned entries are visible but not selectable in live mappings and must not expose a fake confidence slider.
- **UiState.qml / RuntimeController:** expose backend-driven `gestureCatalog`, `mappingRows` trigger metadata and editing APIs; stop duplicating hardcoded gesture lists in QML.
- **Components / theme:** reuse `VcCard`, `VcComboBox`, `VcButton`, `DisclosurePanel`, `GestureGlyph`, `StatusBadge` and theme token colors. Update `app/i18n/pl.json`. Preserve dark/light mode, DPI responsiveness, and one-outer-scroll-area behavior where already established.
- **No fake preview interactions:** sequence appears explicitly unavailable, gamepad actions remain visually unavailable until a backend supports them.

## 7. Code change map

**New proposed components:**
- `app/src/gestures/GestureRegistry.hpp/.cpp`
- `app/src/gestures/TriggerCombinationEngine.hpp/.cpp` (or `TwoHandCombinationEngine`)
- `tests/TriggerCombinationTests.cpp`, profile-migration test, QML contract checks

**Extend:**
- `app/src/mapping/MappingTypes.hpp` — generic trigger specification and Any side
- `app/src/mapping/ActionMapper.hpp/.cpp` — normalized resolved trigger events
- `app/src/mapping/MappingValidator.hpp` — duplicate pairs, pair/single conflict warnings, timing and availability checks
- `app/src/mapping/ProfileStore.hpp/.cpp` — migration + v2 serialization
- `app/src/runtime/RuntimeController.hpp/.cpp` — wire between per-hand state manager and mapper; expose QML properties / methods and safe resets
- `app/qml/pages/MappingPage.qml`, `app/qml/pages/GesturesPage.qml`, `app/qml/UiState.qml`, optional `GestureGlyph.qml`
- `app/i18n/pl.json`, `app/CMakeLists.txt`, `tests/CMakeLists.txt`, release docs

Avoid changing input backends, MediaPipe, camera or continuous cursor handling unless regression testing reveals a necessary defect.

## 8. Acceptance and regression test matrix

- Left/right permutations, including `FIST+POINT` vs `POINT+FIST`, one-handed vs two-handed, and an empty/disabled pair.
- Both hands already stable; left precedes right; right precedes left; just inside/outside timing tolerance; overlapped hold shorter/longer than threshold.
- Single press/hold/toggle conflicts, pair press/hold/toggle, priority against existing cursor movement gate and freeze; no duplicate output.
- Loss of left, loss of right, `trackId` replacement and `None` classification; immediate release even when hand dropout grace is enabled.
- `Any` hand tie and loss/re-entry; one action only when both hands match.
- STOP, F8, output pause, profile change, profile edit/reload, pipeline/model reload, camera unplug/reconnect; never leave a held key/mouse button.
- v1->v2 roundtrip across Default/Desktop/Game/Presentation + user custom profiles; legacy mappings byte-semantically equivalent after migration; atomic save, validation of malformed v2, no data loss.
- Test with only four real available gestures while catalog includes all future entries; unavailable gestures cannot fire or appear as supported.
- Existing native tests, existing Python/ML tests, and Windows GUI smoke tests (Qt Creator build/run/debug, dark/light, PL/EN, narrow/wide layouts, DPI, camera).
- Monitor output latency and preserve the current latest-frame/backpressure policy and full-FPS event pipeline.

## 9. Delivery slices

1. **Foundation:** registry, model compatibility tests, v2 parser/migration, tests.
2. **Core runtime:** pair engine + arbitration + held-output safety + tests, while keeping UI unchanged.
3. **Mapping UI:** modal modes, left/right selectors, list items, validation, localization, responsive check.
4. **Integration:** Qt Creator Windows smoke tests, live camera, profile import/export roundtrips, performance and regression review.

No source modifications were performed during this review.
