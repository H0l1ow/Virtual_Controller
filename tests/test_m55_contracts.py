"""M5.5 source-level guardrails; full Qt/Windows end-to-end testing is still required."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def src(path):
    return (ROOT / path).read_text(encoding='utf-8')


class PreMlReadiness(unittest.TestCase):
    def test_safety_all_terminal_paths(self):
        runtime = src('app/src/runtime/RuntimeController.cpp')
        output = src('app/src/input/OutputService.cpp')
        for snippet in ('void RuntimeController::requestSessionStop(',
                        'void RuntimeController::handleWorkerFault(',
                        'void RuntimeController::stop()',
                        'bool RuntimeController::loadProfile('):
            self.assertIn(snippet, runtime)
        self.assertIn('outputService_.stop();', runtime)
        self.assertIn('hasBlockingMappingIssues(activeMappingProfile_)', runtime)
        self.assertIn('guard_.stop();', output)
        self.assertIn('backend->releaseAll();', output)
        self.assertIn('setOutputArmed(false)', runtime)

    def test_responsive_ui(self):
        row = src('app/qml/components/SettingsRow.qml')
        button = src('app/qml/components/VcButton.qml')
        mapping = src('app/qml/pages/MappingPage.qml')
        self.assertIn('width < 670', row)
        self.assertIn('labelText.implicitWidth', button)
        self.assertIn('visible: root.width >= 1180', mapping)
        self.assertIn('Math.min(360', src('app/qml/pages/GesturesPage.qml'))

    def test_pip_monitor_and_original_restore_behavior(self):
        prefs = src('app/src/runtime/AppPreferences.cpp')
        qml = src('app/qml/components/MiniPreviewWindow.qml')
        self.assertIn('QGuiApplication::screens()', prefs)
        self.assertIn('QStringLiteral("pipScreenName")', prefs)
        self.assertIn('screen: AppPreferences.pipScreen', qml)
        self.assertIn('mini.restoreRequested()', qml)
        self.assertNotIn('showFullScreen()', qml)

    def test_camera_recovery_and_input_state(self):
        runtime = src('app/src/runtime/RuntimeController.cpp')
        self.assertIn('cameraReconnectPolicy_.begin(deviceId.toStdString(), pollLimit)', runtime)
        self.assertIn('Camera reconnected - waiting for tracking', runtime)
        self.assertIn('camera_->start()', runtime)

    def test_hand_range_calibration_uses_real_landmarks(self):
        backend = src('app/src/runtime/RuntimeController.cpp')
        hand = src('app/src/gestures/HandRangeCalibrator.cpp')
        self.assertIn('rangeCalibrator_.ingest(frame)', backend)
        self.assertIn('config.movementScaleX', backend)
        self.assertIn('appliedPinchLow_[side].load()', backend)
        self.assertIn('extractGestureGeometry(hand, frame.width, frame.height)', hand)
        self.assertIn('setHandCalibration(saved.handCalibration)', src('app/qml/UiState.qml'))
        self.assertNotIn('outputArmed:', src('app/qml/UiState.qml').split('function settingsSnapshot()')[1].split('function restoreSavedSettings()')[0])

    def test_cursor_test_isolated(self):
        backend = src('app/src/runtime/RuntimeController.cpp')
        self.assertIn('setOutputArmed(false);', backend)
        self.assertIn('if (!playgroundMode_)\n        outputService_.submit(', backend)
        self.assertIn('cursorPlayground_.update(', backend)

    def test_mapping_conflicts(self):
        checker = src('app/src/mapping/MappingValidator.hpp')
        self.assertIn('drag_exclusive', checker)
        self.assertIn('duplicate_id', checker)
        self.assertIn('hasBlockingMappingIssues', checker)

    def test_reload_is_real_session_restart(self):
        backend = src('app/src/runtime/RuntimeController.cpp')
        self.assertIn('bool RuntimeController::reloadGestureModel()', backend)
        self.assertIn('pendingModelReload_ = true;', backend)
        self.assertIn('requestSessionStop(RuntimeState::Stopped);', backend)
        self.assertIn('reloadQueued_ = true;', backend)
        self.assertIn('onClicked: root.uiState.reloadGestureModel()', src('app/qml/pages/MLPage.qml'))

    def test_distinct_performance_measurements(self):
        backend = src('app/src/runtime/RuntimeController.cpp')
        metrics = src('app/src/runtime/RuntimeMetrics.cpp')
        self.assertIn('gestureEndUs - gestureStartUs', backend)
        self.assertIn('recordOutputDispatch', backend)
        for name in ('cameraIntervalP50Ms', 'mediaPipeP50Ms', 'gestureP50Ms', 'outputDispatchP50Ms'):
            self.assertIn(name, metrics)

    def test_replay_without_system_actions(self):
        backend = src('app/src/runtime/RuntimeController.cpp')
        block = backend.split('void RuntimeController::tickGestureReplay()')[1].split('void RuntimeController::updateStats()')[0]
        self.assertIn('replayEngine_.update(frame, thresholds)', block)
        self.assertNotIn('outputService_.submit', block)
        self.assertNotIn('outputService_.arm', block)
        stop = backend.split('void RuntimeController::stop()')[1].split('void RuntimeController::togglePipeline()')[0]
        self.assertIn('stopGestureReplay();', stop)
        self.assertIn('GestureReplayReader', src('app/CMakeLists.txt'))
        self.assertIn('QtQuick.Dialogs', src('app/qml/pages/MLPage.qml'))

    def test_translations(self):
        d = json.loads(src('app/i18n/pl.json'))
        for phrase in ('Mini preview monitor', 'Gesture Replay (offline)',
                       'Performance Monitor (median / P95)', 'Apply PINCH',
                       'Reload model'):
            self.assertTrue(d.get(phrase), phrase)


if __name__ == '__main__':
    unittest.main()
