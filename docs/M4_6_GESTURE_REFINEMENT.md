# M4.6 Gesture Recognition Refinement (0.9.6)

## Scope

M4.6 refines the Rules recognizer and centralizes the gesture catalog. It does
not change GestureStateManager semantics, ActionMapper semantics, cursor control
or Windows output safety.

## Gesture catalog

The backend owns one catalog of 21 mapping gestures. `NONE` remains a technical
recognizer state and is not map-able.

### Active static Rules gestures (13)

- FIST
- OPEN_HAND
- POINT
- PINCH
- THUMB_UP
- THUMB_DOWN
- VICTORY
- OK
- I_LOVE_YOU
- ROCK
- CALL_ME
- THREE_FINGERS
- FOUR_FINGERS

### Reserved temporal gestures (8)

- SWIPE_LEFT
- SWIPE_RIGHT
- SWIPE_UP
- SWIPE_DOWN
- CIRCLE_CW
- CIRCLE_CCW
- PUSH
- PULL

Reserved temporal gestures are visible in Mapping so profile IDs are stable, but
an enabled mapping is rejected until temporal recognition exists. They can be
stored disabled for future profiles.

## PINCH changes

The previous Rules implementation used a full XYZ thumb-index distance and gave
FIST priority. A natural pinch with the remaining fingers curled could therefore
be suppressed as FIST.

M4.6 changes this behavior:

1. The ONNX feature vector stays `vc.hand134.v2`; no feature-schema migration is
   required.
2. Rules use a separate thumb/index distance with full XY and only 0.20 weight
   on Z.
3. PINCH requires the index/thumb not to be deeply curled, preventing a closed
   fist from becoming a false pinch.
4. Clear thumb/index contact is resolved before FIST and other posture-only
   rules.
5. The GestureEngine uses a Schmitt-style latch: PINCH enters at a tighter
   distance and releases at a wider one.
6. Score smoothing weights the current frame at 0.70 instead of 0.45. M4
   debounce/cooldown still remains the event-level stability layer.

## Static gesture geometry

Rules now use explicit thumb curl/extension/direction plus the existing
index/middle/ring/little curl values. Thumb-only metrics are rule-only and do not
change the 134-value temporal feature contract.

The static rules are intentionally deterministic fallback logic. They are not a
replacement for a trained temporal model and should be tuned against real
camera captures.

## ONNX compatibility

The optional ONNX model remains five-class:

```text
NONE, FIST, OPEN_HAND, POINT, PINCH
```

M4.6 maps those five outputs into the expanded score vector and merges them with
Rules. The application therefore does not pretend that an untrained 22-class
TCN exists.

## UI / Mapping

`MappingPage.qml` no longer contains its own hard-coded gesture list. It reads
`mappingGestureOptions` from RuntimeController. `GesturesPage` reads the same
backend catalog through `gestureCatalog`.

Dynamic entries are labelled `Temporal / planned`. Selecting one in Mapping
forces the row disabled; C++ also rejects attempts to save it enabled.

## Tests

`vc_gesture_recognition_tests` covers:

- all 13 active static gestures,
- exactly 21 unique mapping gesture definitions,
- 13 static + 8 temporal capability split,
- PINCH with curled middle/ring/little fingers,
- PINCH depth-noise tolerance,
- PINCH hysteresis enter/hold/release behavior,
- causal 16-frame feature window,
- left/right feature canonicalization,
- engine threshold/reset behavior.
