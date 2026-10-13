"""M5.3 static integration checks where Qt/MSVC cannot be compiled on CI Linux."""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(relative):
    return (ROOT / relative).read_text(encoding="utf-8")


class GestureTuningContracts(unittest.TestCase):
    def test_threshold_is_thread_safe_and_profile_schema_unchanged(self):
        impl = load("app/src/runtime/RuntimeController.cpp")
        header = load("app/src/runtime/RuntimeController.hpp")
        engine = load("app/src/gestures/GestureEngine.cpp")
        self.assertIn("gestureThresholdOverrides_[idx].load()", impl)
        self.assertIn("GestureThresholds thresholds", impl)
        self.assertIn("thresholds.forClass(bestClass)", engine)
        self.assertIn("std::array<std::atomic_int, kGestureClassCount>", header)
        self.assertIn('gestureThresholdOverrides: gestureThresholdOverrides', load("app/qml/UiState.qml"))
        self.assertIn('setOutputArmed(false)', load("app/qml/UiState.qml"))
        self.assertNotIn("gestureThresholdOverrides", load("app/src/mapping/ProfileStore.cpp"))
        self.assertEqual(134, int(re.search(r'kGestureFeatureCount\s*=\s*(\d+)',
                          load("app/src/gestures/HandFeatures.hpp")).group(1)))

    def test_calibration_disarms_and_does_not_auto_apply(self):
        impl = load("app/src/runtime/RuntimeController.cpp")
        calibrator = load("app/src/gestures/GestureCalibrator.cpp")
        self.assertIn("setOutputArmed(false);\n    calibrator_.start", impl)
        self.assertIn("calibrator_.capturing()", impl)
        self.assertIn("calibrator_.ingest(gestures)", impl)
        self.assertIn("calibrator_.suggestedThreshold", impl)
        self.assertIn('CalibrationPanel {', load("app/qml/pages/SettingsPage.qml"))
        self.assertIn('kRequiredSamples = 36', load("app/src/gestures/GestureCalibrator.hpp"))
        self.assertIn("dominance < 75", calibrator)

    def test_cursor_playground_cannot_arm_or_generate_os_events(self):
        impl = load("app/src/runtime/RuntimeController.cpp")
        begin = impl.index('void RuntimeController::setOutputArmed(')
        end = impl.index('void RuntimeController::setSensitivity(', begin)
        guard = impl[begin:end]
        self.assertIn('playgroundMode_', guard)
        self.assertIn('calibrator_.capturing()', guard)
        self.assertIn('outputService_.stop()', guard)
        self.assertIn('if (!playgroundMode_)', impl)
        self.assertIn('outputService_.submit', impl)
        self.assertIn('result.controller.mouseX', impl)
        self.assertIn('root.uiState.stopCursorPlayground()', load('app/qml/components/CursorPlaygroundPage.qml'))

    def test_qml_and_translations_registered(self):
        cmake = load('app/CMakeLists.txt')
        for name in ('CalibrationPanel.qml', 'CursorPlaygroundPage.qml',
                     'GestureCalibrator.cpp', 'CursorPlayground.cpp'):
            self.assertIn(name, cmake)
        english = ('Calibration', 'Cursor Playground', 'Capture samples',
                   'Back to gesture tests', 'Recommended thresholds')
        catalog = json.loads(load('app/i18n/pl.json'))
        for term in english:
            self.assertTrue(catalog.get(term), term)


if __name__ == '__main__':
    unittest.main()
