"""M6.3.4 GUI contracts. Static QML checks; real Qt/Windows smoke still needed."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / 'app' / 'qml'

def read(path):
    return (QML / path).read_text(encoding='utf-8')


class M634Cleanup(unittest.TestCase):
    def test_dashboard_start_only_in_main_action_row(self):
        top = read('components/AppTopBar.qml')
        controller = read('pages/ControllerPage.qml')
        self.assertIn('visible: root.windowRef.currentPage !== 0', top)
        self.assertIn('root.uiState.togglePipeline()', controller)
        self.assertIn('onClicked: gestureDrawer.open()', controller)
        self.assertNotIn('onClicked: root.navigateRequested(2)', controller)

    def test_status_inside_camera_is_narrower_taller_and_larger(self):
        camera = read('components/CameraStage.qml')
        side = read('components/DashboardSidePanel.qml')
        self.assertIn('DashboardSidePanel {', camera)
        self.assertIn('width: Math.min(222', camera)
        self.assertIn('height: Math.min(385', camera)
        self.assertIn('translucentOpacity: theme.isLight ? 0.78 : 0.68', side)
        self.assertIn('font.pixelSize: root.compact ? 13 : 16', side)
        self.assertIn('root.uiState.setOutputArmed(checked)', side)
        self.assertNotIn('ScrollView {', read('pages/ControllerPage.qml').split('Drawer {')[0])

    def test_compact_functional_gesture_drawer(self):
        controller = read('pages/ControllerPage.qml')
        drawer = read('components/DashboardGestureDrawer.qml')
        self.assertIn('DashboardGestureDrawer {', controller)
        self.assertIn('edge: Qt.RightEdge', drawer)
        self.assertIn('startGestureCalibration(root.selectedHand, root.selectedGesture)', drawer)
        self.assertIn('cancelGestureCalibration()', drawer)
        self.assertIn('applyGestureCalibration(root.selectedGesture)', drawer)
        self.assertIn('setGestureThreshold(thresholdRow.modelData, value)', drawer)
        self.assertIn('root.uiState.saveSettings()', drawer)
        self.assertIn('onFullSettingsRequested: root.navigateRequested(2)', controller)
        self.assertIn('qml/components/DashboardGestureDrawer.qml', (ROOT / 'app/CMakeLists.txt').read_text())
        self.assertEqual(drawer.count('CameraStage {'), 0)
        self.assertEqual(drawer.count('VideoOutput {'), 0)

    def test_gesture_hints_and_hand_badges_only_available_fullscreen(self):
        camera = read('components/CameraStage.qml')
        gestures = read('pages/GesturesPage.qml')
        controller = read('pages/ControllerPage.qml')
        self.assertIn('visible: root.immersiveMode && root.showGestureHints', camera)
        live = gestures.split('CameraStage {')[1].split('onFullscreenRequested:')[0]
        self.assertIn('showGestureHints: false', live)
        self.assertIn('showHandBadges: false', live)
        self.assertNotIn('model: [\n                                    { name: qsTr("Left hand")', gestures)
        self.assertIn('showGestureHints: root.showGestureHints', controller)
        self.assertIn('onOptionRequested: function(key, visible)', controller)

    def test_gamepad_layouts_complete_but_no_false_live_status(self):
        visual = read('components/GamepadVisual.qml')
        page = read('pages/GamepadPage.qml')
        state = read('UiState.qml')
        for label in ('LT', 'RT', 'LB', 'RB', 'L1', 'L2', 'R1', 'R2',
                      'LS', 'RS', 'L3', 'R3', 'View', 'Home', 'Menu',
                      'Create', 'PS', 'Options'):
            self.assertIn('"' + label + '"', visual)
        for symbol in ('△', '○', '×', '□', '↑', '↓', '←', '→'):
            self.assertIn(symbol, visual)
        self.assertIn('Touchpad', visual)
        self.assertIn('property bool mockData: false', page)
        self.assertIn('mockData: window.mockUi', read('Main.qml'))
        self.assertIn('root.mockData ? root.uiState.leftStickValue : qsTr("N/A")', page)
        self.assertIn('model: root.mockData ? root.uiState.gamepadInputs : []', page)
        self.assertNotIn('statusText: root.uiState.pipelineRunning ? qsTr("Live")', page)
        self.assertIn('readonly property string leftStickValue: "0.00 / 0.00"', state)

    def test_only_qml_files_in_resources(self):
        cmake = (ROOT / 'app/CMakeLists.txt').read_text(encoding='utf-8')
        for path in QML.rglob('*.qml'):
            rel = path.relative_to(ROOT / 'app').as_posix()
            self.assertIn(rel, cmake, rel)


if __name__ == '__main__':
    unittest.main()
