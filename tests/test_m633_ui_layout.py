"""M6.3.3: contracts for approved layout changes; Qt runtime smoke still required."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / 'app/qml'
def read(path): return (QML / path).read_text(encoding='utf8')

class M633Layout(unittest.TestCase):
    def test_camera_status_is_a_translucent_child_of_video_stage(self):
        controller = read('pages/ControllerPage.qml')
        camera = read('components/CameraStage.qml')
        overlay = read('components/DashboardSidePanel.qml')
        self.assertNotIn('id: dashboardScroll', controller)
        self.assertIn('dashboardStatusOverlay: true', controller)
        self.assertIn('DashboardSidePanel {', camera)
        self.assertIn('anchors.verticalCenter: parent.verticalCenter', camera)
        self.assertIn('translucent: true', overlay)
        self.assertIn('root.uiState.setOutputArmed(checked)', overlay)
        self.assertIn('root.uiState.activeProfile', overlay)
        self.assertIn('root.uiState.leftTracked', overlay)
        self.assertIn('root.uiState.rightTracked', overlay)
        self.assertIn('root.uiState.outputActive', overlay)
        self.assertNotIn('ScrollView {', overlay)
        self.assertNotIn('DashboardSidePanel {', controller)
        self.assertIn('showGestureHints: false', controller)
        self.assertIn('Layout.fillHeight: true', controller)

    def test_camera_sink_is_reused_and_minimize_persistence_preserved(self):
        camera = read('components/CameraStage.qml')
        main = read('Main.qml')
        self.assertEqual(camera.count('VideoOutput {'), 1)
        self.assertIn('root.uiState.attachVideoOutput(videoOutput.videoSink)', camera)
        self.assertIn('root.uiState.attachVideoOutput(null)', camera)
        self.assertIn('window.uiState.setMainWindowMinimized(presentationSuspended)', main)
        self.assertIn('MiniPreviewWindow {', main)

    def test_one_scroll_region_for_advanced_gestures(self):
        gestures = read('pages/GesturesPage.qml')
        self.assertEqual(gestures.count('ScrollView {'), 1)
        self.assertIn('property int activeSection: 0', gestures)
        self.assertIn('root.activeSection === 1', gestures)
        self.assertIn('root.activeSection === 2', gestures)
        for panel in ('CalibrationPanel.qml', 'GestureTuningPanel.qml'):
            component = read('components/' + panel)
            self.assertNotIn('ScrollView {', component)
            self.assertIn('implicitHeight:', component)
        self.assertIn('root.uiState.setGestureThreshold(gestureCard.modelData.code, value)', gestures)

    def test_ml_recording_files_model_offline_order(self):
        ml = read('pages/MLPage.qml')
        anchors = ('RecordingCapturePanel {', 'RecordingFilesPanel {', 'Recognition model', 'id: offlineTools')
        self.assertEqual(list(map(ml.index, anchors)), sorted(map(ml.index, anchors)))
        self.assertIn('reloadGestureModel()', ml)
        self.assertIn('startGestureReplay(selectedFile.toString())', ml)
        self.assertNotIn('startTraining(', ml)

    def test_xbox_and_playstation_have_distinct_shoulders_and_triggers(self):
        visual = read('components/GamepadVisual.qml')
        page = read('pages/GamepadPage.qml')
        for tag in ('LT', 'RT', 'LB', 'RB', 'L1', 'L2', 'R1', 'R2'):
            self.assertIn('"' + tag + '"', visual)
        for tag in ('L3', 'R3', 'LS', 'RS', 'View', 'Menu', 'Create', 'Options', 'Home', 'PS'):
            self.assertIn('"' + tag + '"', visual)
        for glyph in ('△', '○', '×', '□'):
            self.assertIn(glyph, visual)
        self.assertIn('dpadCanvas', visual)
        self.assertIn('Touchpad', visual)
        self.assertIn('property string controllerLayout: "xbox"', page)
        self.assertIn('controllerLayout: root.controllerLayout', page)
        self.assertIn('Mock backend', page)
        self.assertIn('Display layout only', page)
        self.assertNotIn('setGamepadBackend(', page)

if __name__ == '__main__': unittest.main()
