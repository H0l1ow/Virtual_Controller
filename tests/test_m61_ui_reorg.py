"""M6.1 navigation contracts; full Qt Creator/Windows smoke-test is still required."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / 'app' / 'qml'


def text(rel):
    return (ROOT / rel).read_text(encoding='utf-8')


class UiNavigationContracts(unittest.TestCase):
    def test_tabs_map_one_to_one_to_stack_pages(self):
        main = text('app/qml/Main.qml')
        side = text('app/qml/components/AppSideBar.qml')
        pages = re.findall(r'\b(ControllerPage|MappingPage|GesturesPage|TestPage|GamepadPage|MLPage|SettingsPage)\s*\{', main)
        self.assertEqual(pages, [
            'ControllerPage', 'MappingPage', 'GesturesPage', 'TestPage',
            'GamepadPage', 'MLPage', 'SettingsPage'
        ])
        for i in (0, 1, 2, 5, 6):
            self.assertIn('page: '+str(i)+',', side)
        self.assertIn('{ page: 3, title: qsTr("Test lab")', side)
        self.assertIn('{ page: 4, title: qsTr("Gamepad monitor")', side)
        self.assertIn('currentIndex: window.currentPage', main)
        self.assertIn('onPageSelected: window.selectPage(index)', main)
        self.assertIn('onClicked: root.openPage(nav.modelData.page)', side)

    def test_ml_controls_exist_only_in_ml_page(self):
        ml = text('app/qml/pages/MLPage.qml')
        settings = text('app/qml/pages/SettingsPage.qml')
        gestures = text('app/qml/pages/GesturesPage.qml')
        self.assertIn('reloadGestureModel()', ml)
        self.assertIn('models/hand_landmarker.task', ml)
        recorder = text('app/qml/components/RecordingCapturePanel.qml')
        self.assertIn('RecordingCapturePanel {', ml)
        self.assertIn('startGestureRecording(root.leftRecordLabel, root.rightRecordLabel)', recorder)
        self.assertIn('RecordingFilesPanel {', ml)
        self.assertIn('stopGestureRecording()', recorder)
        self.assertIn('startGestureReplay(selectedFile.toString())', ml)
        self.assertIn('stopGestureReplay()', ml)
        self.assertIn('"UNLABELED", "NONE", "FIST", "OPEN_HAND", "POINT", "PINCH"', recorder)
        self.assertNotIn('reloadGestureModel()', settings)
        self.assertNotIn('startGestureRecording(', gestures)
        self.assertNotIn('startGestureReplay(', gestures)
        self.assertIn('Training runs in Python, outside Qt Creator', ml)
        self.assertNotIn('startTraining(', ml)

    def test_settings_are_grouped_without_duplicate_model_or_calibration(self):
        settings = text('app/qml/pages/SettingsPage.qml')
        labels = re.findall(r'^            title: qsTr\("([^"]+)"\),', settings, re.M)
        self.assertEqual(labels, ['General', 'Camera', 'Tracking', 'Cursor', 'Desktop'])
        self.assertIn('AppPreferences.pipSizeMode', settings)
        self.assertIn('AppPreferences.pipScreenName', settings)
        self.assertIn('LanguageManager.setLanguageByIndex(index)', settings)
        self.assertIn('root.uiState.setSensitivity(value)', settings)
        self.assertNotIn('CalibrationPanel {', settings)
        self.assertNotIn('gesture_model.onnx', settings)
        self.assertNotIn('hand_landmarker.task', settings)
        self.assertNotIn('settingsProfileCombo', settings)

    def test_gesture_tuning_and_calibration_are_in_gestures(self):
        gestures = text('app/qml/pages/GesturesPage.qml')
        tuning = text('app/qml/components/GestureTuningPanel.qml')
        self.assertIn('GestureTuningPanel {', gestures)
        self.assertIn('CalibrationPanel {', gestures)
        self.assertIn('setRecognitionThreshold(value)', tuning)
        self.assertIn('setGestureThreshold(thresholdSettingRow.modelData.code, value)', tuning)
        self.assertIn('setCooldownMs(parseInt(currentText))', tuning)
        self.assertIn('root.uiState.saveSettings()', gestures)
        self.assertIn('onMoved: root.uiState.setGestureThreshold(gestureCard.modelData.code, value)', gestures)

    def test_performance_moved_to_test_and_pip_unchanged(self):
        test = text('app/qml/pages/TestPage.qml')
        perf = text('app/qml/components/PerformancePanel.qml')
        pip = text('app/qml/components/MiniPreviewWindow.qml')
        self.assertIn('PerformancePanel {', test)
        self.assertIn('root.diagnosticsMode = true', test)
        self.assertIn('mediaPipeP50Ms', perf)
        self.assertIn('outputDispatchP95Ms', perf)
        self.assertIn('onPressed: mini.startSystemMove()', pip)
        self.assertIn('onDoubleClicked: mini.restoreRequested()', pip)
        self.assertNotIn('showFullScreen()', pip)

    def test_components_registered_and_no_model_training_fake(self):
        cmake = text('app/CMakeLists.txt')
        for name in ('MLPage.qml', 'GestureTuningPanel.qml', 'PerformancePanel.qml'):
            self.assertIn(name, cmake)
        self.assertRegex(text('CMakeLists.txt'), r'project\(VirtualController VERSION 0\.(?:14|15)\.\d+ LANGUAGES CXX\)')


if __name__ == '__main__':
    unittest.main()
