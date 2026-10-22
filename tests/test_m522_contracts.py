"""M5.2.2 source contract checks (no Qt SDK required)."""
from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[1]

def get(name):
    return (ROOT/name).read_text(encoding='utf-8')


class StabilityAndPiPTests(unittest.TestCase):
    def test_camera_recovery_is_bounded_when_off_and_persistent_when_requested(self):
        cpp = get('app/src/runtime/RuntimeController.cpp')
        policy = get('app/src/runtime/CameraReconnectPolicy.hpp')
        self.assertIn('cameraRecoveryTimer_.setInterval(1000)', cpp)
        self.assertIn('const int pollLimit = persistent ? std::numeric_limits<int>::max() : 30;', cpp)
        self.assertIn('cameraReconnectPolicy_.begin(deviceId.toStdString(), pollLimit)', cpp)
        self.assertIn('device.id().toStdString() == wanted', cpp)
        self.assertIn('if (exitRequested_)', cpp)
        self.assertIn('outputService_.pause("Camera or pipeline temporarily unavailable")', cpp)
        self.assertIn('start();', cpp)
        self.assertIn('outputService_.stop();', cpp)
        self.assertIn('if (cameraReconnectPolicy_.pending())', cpp)
        self.assertIn('cancelCameraRecovery();', cpp)
        self.assertNotIn('outputService_.arm();', cpp[cpp.index('void RuntimeController::pollCameraRecovery()'):cpp.index('void RuntimeController::refreshDevices()')])
        self.assertIn('Decision::TimedOut', policy)
        self.assertIn('Decision::Restart', policy)

    def test_pip_does_not_change_click_behavior(self):
        pip = get('app/qml/components/MiniPreviewWindow.qml')
        self.assertIn('onPressed: mini.startSystemMove()', pip)
        self.assertIn('onDoubleClicked: mini.restoreRequested()', pip)
        self.assertIn('onClicked: mini.restoreRequested()', pip)
        self.assertEqual(2, pip.count('mini.restoreRequested()'))
        self.assertNotIn('mini.showFullScreen()', pip)
        self.assertIn('AppPreferences.pipSizeMode', pip)
        self.assertIn('AppPreferences.pipShowLandmarks', pip)
        self.assertIn('AppPreferences.setPipPosition', pip)
        self.assertIn('attachPiPVideoSink(videoSink)', pip)
        self.assertNotIn('QCamera {', pip)

    def test_persisted_settings_keep_output_disarmed(self):
        ui = get('app/qml/UiState.qml')
        preferences = get('app/src/runtime/AppPreferences.cpp')
        self.assertIn('autoRecoverCamera: autoRecoverCamera', ui)
        self.assertIn('pipSizeMode: AppPreferences.pipSizeMode', ui)
        self.assertIn('pipX: AppPreferences.pipX', ui)
        self.assertIn('setOutputArmed(false)', ui)
        self.assertIn('QSettings settings', preferences)
        self.assertIn('QStringLiteral("pipShowLandmarks")', preferences)
        self.assertNotIn('snapshot.insert(QStringLiteral("outputArmed")', preferences)

    def test_polish_catalog_covers_new_ui(self):
        catalog=json.loads(get('app/i18n/pl.json'))
        for title in ('Mini preview size', 'Mini preview position', 'Show landmarks in mini preview',
                      'Camera auto-reconnect', 'Waiting for camera reconnection',
                      'Camera reconnection timed out', 'Camera reconnected; output disarmed',
                      'Camera unavailable', 'Reset position'):
            self.assertTrue(catalog[title],title)
        settings = get('app/qml/pages/SettingsPage.qml')
        self.assertIn('generalScroll', settings)
        self.assertIn('cameraScroll', settings)
        self.assertIn('ScrollBar.horizontal.policy: ScrollBar.AlwaysOff', settings)
        self.assertIn('setAutoRecoverCamera(checked)', settings)
        self.assertIn('AppPreferences.pipSizeMode', settings)

    def test_small_window_navigation_remains_accessible(self):
        main = get('app/qml/Main.qml')
        side = get('app/qml/components/AppSideBar.qml')
        settingsRow = get('app/qml/components/SettingsRow.qml')
        self.assertIn('width < 1180', main)
        self.assertIn('onCollapseToggled:', main)
        self.assertIn('ToolTip.text: nav.modelData.title', side)
        self.assertIn('implicitWidth: collapsed ? 67 : 218', side)
        self.assertIn('columns: root.compact ? 1 : 2', settingsRow)
        self.assertIn('minimumWidth: 780', main)


if __name__ == '__main__':
    unittest.main()
