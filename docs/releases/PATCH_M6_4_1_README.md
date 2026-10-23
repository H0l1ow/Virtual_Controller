# VirtualController 0.16.1 — M6.4.1 Gesture Engine Expansion

**Base:** 0.16.0 (M6.4.0). **Target:** Windows 10/11 x64, Qt 6.11.2,
MSVC 2022 x64, CMake. This archive contains project sources, the MediaPipe
native runtime and the bundled hand model, **not a prebuilt 0.16.1 EXE**.

## Installation (Qt Creator)

1. Back up the existing `VirtualController` project folder and custom profiles.
2. Extract the full `VirtualController_M6_4_1_FULL.zip` into a workspace, or
   extract `VirtualController_M6_4_1_PATCH.zip` into the **parent folder**
   containing your existing `VirtualController/` directory. The archive
   includes that top-level directory; overwrite matching files. Avoid copying an old
   generated `build/` folder into the new clean copy.
3. Open the root `VirtualController/CMakeLists.txt` in Qt Creator. Select
   **Qt 6.11.2 MSVC 2022 64-bit**. Reconfigure CMake, then Clean/Rebuild All.
4. Launch the `VirtualController` target and verify the Mapping and Gestures
   pages. MediaPipe and the hand model are bundled in the full archive; ONNX
   inference stays optional (`VC_WITH_ONNX=OFF` unless configured manually).
5. The installed user profiles are stored under Qt's
   `QStandardPaths::AppConfigLocation/profiles` directory; they are **not**
   overwritten by unpacking sources or first-run profile seeding.

## Added in M6.4.1

- One classifier-independent catalog of 26 gesture definitions (12 static,
  9 dynamic and 5 temporal). **Only four static gestures** (`FIST`,
  `OPEN_HAND`, `POINT`, `PINCH`) are presently recognized. Future classes are
  clearly tagged `Planned`; they cannot be mapped as active triggers.
- Ordered **left + right** combinations without training a separate class for
  every pair. The currently supported four classes give **16** combinations;
  a future set of 12 static classes will give 144.
- Configurable `synchronizationMs` (default 250) and `overlapHoldMs` (150),
  both adjustable between 0 and 1500 ms in the Mapping editor.
- A deterministic arbitration layer before ActionMapper, so the confirmed
  `FIST + FIST` trigger fires the pair action and not either FIST singleton.
  Single-hand action presses/holds are deferred while a pair could still
  form. On failure, single mappings resume; with no eligible enabled pair,
  the old latency is unchanged.
- Pair HOLD release and output neutralization on hand loss, identity change,
  tracking STOP, profile modification or runtime reset; no ghost singleton
  press on pair breakup. Single-hand `Any` binds one hand at a time.
- `vc.mapping.profile.v2` JSON. Existing v1 profiles are read in memory and
  written as v2 **only after explicit save**. The original file is copied
  once to `<name>.json.v1.bak` before the first migration write. v2 rejects
  unknown trigger fields, unavailable gestures and invalid timing.
- Mapping editor stays within the current side popup and Vc design system.
  Trigger choice: Single / Both hands / Sequence (unavailable). Gestures now
  includes a scrollable-page library showing available vs planned gestures.
  Existing sliders continue to control only genuinely recognized classes.

## Test it

1. Start tracking and open **Mapping**. Click **Add mapping**.
2. Choose **Both hands**; set **Left hand gesture = FIST**, **Right hand
   gesture = FIST**, **Sync window = 250**, **Overlap hold = 150**.
3. Assign **Key Space**, behavior **Press**, enable and **Save mapping**.
4. Enable system output using the application's existing safety controls.
   Make two fists simultaneously and hold longer than the overlap threshold.
5. Repeat with individual FIST mappings in the profile: the confirmed pair
   must produce only the combined action, not 3 overlapping actions.
6. For safer testing, keep system output disarmed and inspect logical actions
   on the Test page (no OS input is generated while disarmed).

**Not delivered in this stage:** recognition of planned gesture classes,
trained dynamic TCN, sequence controls, spatial two-hand zoom/rotate, OS
screenshot/media-control actions, arbitrary key chords such as Ctrl+C, or a
working Xbox/PlayStation driver. These remain future milestones.

## Validation and limitations

- Linux/headless: **23/23 native CTest** and **115/115 Python tests** passed.
- New native tests cover pair precedence, ordered L/R, timing, hand loss,
  `Any` ownership and unchanged singleton behavior without a configured pair.
- A QtCore test checks v1->v2 roundtrip, atomic backups, and rejection of
  unsupported v2 triggers; it runs automatically in builds with Qt available.
- **Not verified in this environment:** Qt 6.11.2/MSVC build, live webcam,
  Windows input injection, and pixel-level QML layout on multiple resolutions.
  Rebuild and smoke-test on the target Qt Creator kit before production use.
