"""M6.3.1 UI behavior contracts, runnable without the target Windows Qt SDK.

These check API wiring and the five accepted-screen hierarchy. They are not
pixel tests and do not replace a physical-camera GUI smoke test.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / 'app' / 'qml'

def qml(path):
    return (QML / path).read_text(encoding='utf-8')

class M631VisualFidelity(unittest.TestCase):
    def test_five_primary_destinations_and_tools(self):
        side = qml('components/AppSideBar.qml')
        primary = side.split('readonly property var destinations: [')[1].split(']')[0]
        for page in (0, 1, 2, 5, 6):
            self.assertIn(f'page: {page}', primary)
        for page in (3, 4):
            self.assertNotIn(f'page: {page}', primary)
            self.assertIn(f'page: {page}', side)
        self.assertIn('onClicked: root.openPage(toolNav.modelData.page)', side)
        self.assertIn('collapsed ? 67 : 218', side)

    def test_dashboard_uses_real_tracking_and_safe_arming(self):
        page = qml('pages/ControllerPage.qml')
        self.assertIn('CameraStage {', page)
        self.assertIn('DashboardSidePanel {', qml('components/CameraStage.qml'))
        self.assertIn('root.uiState.togglePipeline()', page)
        self.assertIn('root.uiState.setOutputArmed(checked)', qml('components/DashboardSidePanel.qml'))
        self.assertIn('root.uiState.outputArmed', page)
        self.assertIn('ListModel { id: recentEvents }', page)
        self.assertIn('onLeftGestureEventChanged()', page)
        self.assertIn('onRightGestureEventChanged()', page)
        self.assertIn('onClicked: advancedDrawer.open()', page)
        self.assertIn('active: advancedDrawer.visible', page)

    def test_one_active_camera_and_deterministic_fullscreen(self):
        page = qml('pages/ControllerPage.qml')
        stage = qml('components/CameraStage.qml')
        main = qml('Main.qml')
        gestures = qml('pages/GesturesPage.qml')
        self.assertIn('previewActive: root.previewActive && !root.immersiveMode', page)
        self.assertIn('previewActive: root.previewActive && root.immersiveMode', page)
        self.assertIn('Qt.callLater(root.updatePreviewAttachment)', stage)
        self.assertIn('else if (root.sinkAttached)', stage)
        self.assertIn('previewActive: window.currentPage === 2', main)
        self.assertIn('previewActive: window.currentPage === 5', main)
        self.assertIn('onOpenDashboardRequested:', main)
        self.assertIn('window.setControllerFullscreen(true)', main)
        self.assertIn('CameraStage {', gestures)

    def test_gestures_use_real_classifier_thresholds(self):
        page = qml('pages/GesturesPage.qml')
        self.assertEqual(page.count('code: "FIST"'), 1)
        self.assertIn('root.uiState.gestureThresholds[gestureCard.modelData.code]', page)
        self.assertIn('root.uiState.setGestureThreshold(gestureCard.modelData.code, value)', page)
        self.assertIn('root.uiState.gestureThresholdOverrides', page)
        self.assertIn('CalibrationPanel {', page)
        self.assertIn('GestureTuningPanel {', page)
        self.assertNotIn('setGestureEnabled(', page)

    def test_mapping_keeps_stable_protocol_identifiers(self):
        page = qml('pages/MappingPage.qml')
        for s in ('PINCH', 'FIST', 'POINT', 'OPEN HAND', 'Left click', 'Key Space'):
            self.assertIn('"' + s + '"', page)
        self.assertIn('root.uiState.addMapping()', page)
        self.assertIn('root.uiState.updateMapping(', page)
        self.assertIn('root.uiState.removeMapping(index)', page)
        self.assertIn('root.uiState.saveActiveProfile()', page)
        self.assertIn('root.uiState.duplicateActiveProfile(', page)
        self.assertIn('root.uiState.deleteProfile(', page)
        self.assertIn('id: editPopup', page)
        self.assertNotIn('startMacro(', page)

    def test_ml_shows_real_model_and_recording_actions(self):
        page = qml('pages/MLPage.qml')
        capture = qml('components/RecordingCapturePanel.qml')
        files = qml('components/RecordingFilesPanel.qml')
        self.assertIn('reloadGestureModel()', page)
        self.assertIn('RecordingCapturePanel {', page)
        self.assertIn('RecordingFilesPanel {', page)
        self.assertIn('startGestureRecording(root.leftRecordLabel, root.rightRecordLabel)', capture)
        self.assertIn('Layout.column: root.width >= 880 ? 1 : 0', capture)
        self.assertIn('Layout.column: 0', capture)
        self.assertIn('root.uiState.openRecordingsFolder()', files)
        self.assertIn('root.uiState.deleteRecordingFile(', files)
        self.assertNotIn('startTraining(', page)

    def test_settings_preserve_existing_api(self):
        page = qml('pages/SettingsPage.qml')
        self.assertIn('id: sectionItem', page)
        self.assertIn('currentIndex: root.sectionIndex', page)
        self.assertIn('AppPreferences.pipScreenName', page)
        self.assertIn('AppPreferences.pipShowLandmarks', page)
        self.assertIn('LanguageManager.setLanguageByIndex(index)', page)
        self.assertIn('root.uiState.saveSettings()', page)
        self.assertIn('root.uiState.resetSettingsDefaults()', page)
        self.assertIn('DesktopIntegration.globalShortcutsAvailable', page)

    def test_primary_buttons_do_not_have_fixed_translated_widths(self):
        button = qml('components/VcButton.qml')
        self.assertIn('labelText.implicitWidth', button)
        self.assertIn('elide: Text.ElideRight', button)
        self.assertNotIn('implicitWidth: 110', qml('pages/MappingPage.qml'))

    def test_qml_balanced_delimiters(self):
        # Lexical scan skips strings and comments, unlike naive brace counting.
        pairs = {')': '(', ']': '[', '}': '{'}
        for path in QML.rglob('*.qml'):
            data = path.read_text(encoding='utf-8')
            stack = []
            i = 0
            state = 'normal'
            while i < len(data):
                ch = data[i]
                nxt = data[i+1] if i+1 < len(data) else ''
                if state in ('"', "'"):
                    if ch == '\\': i += 2; continue
                    if ch == state: state = 'normal'
                elif state == 'line':
                    if ch == '\n': state = 'normal'
                elif state == 'block':
                    if ch == '*' and nxt == '/': state = 'normal'; i += 1
                elif ch == '/' and nxt == '/': state = 'line'; i += 1
                elif ch == '/' and nxt == '*': state = 'block'; i += 1
                elif ch in ('"', "'"): state = ch
                elif ch in '([{': stack.append((ch, i))
                elif ch in ')]}':
                    self.assertTrue(stack and stack[-1][0] == pairs[ch],
                                    f'{path}: unmatched {ch} at {i}')
                    stack.pop()
                i += 1
            self.assertEqual(stack, [], f'{path}: unclosed delimiters')
            self.assertEqual(state, 'normal', f'{path}: unclosed string/comment')

if __name__ == '__main__': unittest.main()
