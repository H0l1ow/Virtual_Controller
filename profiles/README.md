# Mapping profiles

M4 profiles use schema `vc.mapping.profile.v1` and contain discrete gesture
mappings only. The runtime seeds these files into the user's writable Qt app
configuration directory on first run, then edits the writable copies.

Supported hands: `Left`, `Right`.
Supported gestures: `FIST`, `OPEN_HAND`, `POINT`, `PINCH`.
Supported behaviors: `Press`, `Hold`, `Toggle`.

Actions include mouse buttons, scroll and common keyboard keys. Continuous
right-hand cursor motion remains the M2 control path and is intentionally not a
discrete mapping row.
