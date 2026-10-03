# Virtual Controller - UI prototype v0.4

Fourth UI iteration of the new Virtual Controller application, focused on layout stability, Gamepad cleanup and a slightly more modern visual language.

This version rebuilds the entire interface around the approved dark desktop concept: a top navigation bar, a large camera workspace, compact translucent gesture hints and rectangular graphite panels with green live-status accents.

## What is included

### Application shell

- Frameless desktop window with custom top bar and window controls.
- Top navigation: **Controller / Mapping / Gestures / Gamepad / Settings**.
- Central dark theme in `Theme.qml`.
- Reusable controls for buttons, icons, sliders, switches, combo boxes, badges and panels.
- Dark graphite visual language with subtle corner rounding; no blue action buttons or oversized pill-shaped controls.

### Controller

- Large camera workspace designed for a camera pointed at a desk/hands rather than a face.
- Mock two-hand landmark overlay.
- Compact semi-transparent gesture/action hints on the right side of the camera image.
- LEFT / RIGHT hand status chips with detected gesture and confidence.
- Camera status, FPS and resolution overlays.
- Bottom dashboard with:
  - Cursor Tracking,
  - Controls,
  - Quick Settings.

### Mapping

- Profile selector.
- Mapping table with hand, source, type, logical action, output and state.
- Source filters.
- Mapping details editor.
- Import/export/add mapping UI.

### Gestures

- Gesture library with static, dynamic and two-hand categories.
- Live recognition preview.
- Availability/planned states.
- Placeholder entry point for future custom gesture recording/training.

### Gamepad

- Logical controller monitor.
- Xbox-style controller visualization.
- Live mock values for sticks, triggers and buttons.
- Backend selector and safe-neutralization option.

### Settings

- Internal sections for Camera, Tracking, Control, Gestures and Model.
- UI for camera format, handedness, thresholds, sensitivity, smoothing, deadzone, debounce/cooldown and model paths.
- Model/runtime entries remain mock UI only in this iteration.

## Deliberately not connected yet

This is still a UI-only iteration. There is no real:

- camera stream,
- MediaPipe tracking,
- TCN/ONNX inference,
- mouse/keyboard injection,
- Xbox/PlayStation virtual device backend.

`UiMock.qml` provides the data needed to exercise the interface without coupling the UI to unfinished runtime code.

## Qt Creator

1. Open the root `CMakeLists.txt` with **File -> Open File or Project...**.
2. Select **Qt 6.11.2 MSVC 2022 64-bit**.
3. Let Qt Creator configure CMake.
4. Select the `VirtualController` target.
5. Build / Run / Debug normally from Qt Creator.

The UI currently requires only Qt Core, Gui, Qml, Quick and Quick Controls 2.

## QML code style

- Keep one QML property per line inside object blocks.
- Keep child QML objects on separate lines; do not separate child objects with semicolons.
- Expand delegates, controls and settings rows into normal multi-line blocks.
- Keep compact JavaScript only where it improves clarity; Canvas drawing code should use one statement per line.
- Prefer readable object models with one field per line when a row contains several fields.
- Keep source lines reasonably short so Qt Creator diagnostics point to a specific property or statement.


## v0.3 layout corrections

- Removed the artificial keyboard grid, desk perspective lines and mouse circle from the Controller camera mock.
- Simplified the Cursor Tracking preview: no decorative grid or concentric target circle, only a clean desktop preview and cursor position.
- Reduced and simplified gesture hints so they stay inside the camera preview at the minimum supported window size.
- Made gesture library cards taller and placed the grid inside an explicitly sized scroll content item to prevent icons/text from overlapping adjacent cards.
- Added clipping to gesture glyphs.
- Reworked the gamepad visualization around a fixed aspect ratio and scale factor so controls resize together instead of covering each other.
- Replaced the D-pad made from two overlapping rectangles with a single cross-shaped Canvas path.

## UI revision v0.4

- Reworked the Gamepad Monitor layout so the controller visualization always scales to the available area instead of overflowing into neighbouring UI.
- Rebuilt the gamepad visualization with explicit z-order and a single D-pad shape to avoid overlapping graphics.
- Removed the large gamepad icon from the Controller State header and added safer clipping for cards and icons.
- Added subtle 4-7 px corner radii to cards, controls, overlays, navigation states and gesture tiles.
- Reduced top-navigation widths so the header still fits at the minimum supported window size.
- Added explicit Canvas repaint handling to `VcIcon` to prevent stale icon drawings after property changes or resizing.
