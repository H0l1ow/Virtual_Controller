from pathlib import Path
import re
import unittest

PROJECT = Path(__file__).resolve().parents[1]


def read(path):
    return (PROJECT / path).read_text(encoding="utf-8")


class PersistentOutputMinimizeTest(unittest.TestCase):
    def test_minimization_only_routes_preview(self):
        main = read("app/qml/Main.qml")
        cpp = read("app/src/runtime/RuntimeController.cpp")
        start = cpp.index("void RuntimeController::setMainWindowMinimized(")
        end = cpp.index("void RuntimeController::attachRecordingVideoSink", start)
        impl = cpp[start:end]
        self.assertIn("setMainWindowMinimized(presentationSuspended)", main)
        self.assertIn("window.visibility === Window.Minimized", main)
        self.assertIn("window.visibility === Window.Hidden", main)
        self.assertIn("updatePreviewConnections()", impl)
        for forbidden in ("outputService_.stop()", "outputService_.pause(",
                          "requestSessionStop(", "clearTracking(", "setOutputArmed(false)"):
            self.assertNotIn(forbidden, impl)
        self.assertIn("mainWindowMinimized_", read("app/src/runtime/RuntimeController.hpp"))
        self.assertIn("mainWindowMinimized_", read("app/src/runtime/RuntimeController.cpp"))

    def test_ui_mock_and_state_contract(self):
        self.assertIn("function setMainWindowMinimized(", read("app/qml/UiState.qml"))
        self.assertIn("function setMainWindowMinimized(", read("app/qml/mock/UiMock.qml"))
        self.assertIn("OutputResumePolicy", read("app/src/input/OutputService.hpp"))
        self.assertIn("status_.requested = true;", read("app/src/input/OutputService.cpp"))
        self.assertIn("status_.requested = false;", read("app/src/input/OutputService.cpp"))

    def test_capture_does_not_require_preview(self):
        cpp = read("app/src/runtime/RuntimeController.cpp")
        self.assertIn("captureSession_.setVideoSink(&captureSink_)", cpp)
        self.assertIn("bool(pipSink_), pipActive_, mainWindowMinimized_", cpp)
        self.assertIn("setVideoFrame(QVideoFrame{})", cpp)

    def test_ui_texts_are_localized(self):
        import json
        settings = read("app/qml/pages/SettingsPage.qml")
        ml = read("app/qml/pages/MLPage.qml")
        translations = json.loads(read("app/i18n/pl.json"))
        checks = [
            "Reconnect automatically. If system input was enabled, it stays ON but temporarily pauses until tracking recovers.",
            "Restart tracking and reload the model. Enabled system input pauses safely and resumes after tracking stabilizes.",
        ]
        for label in checks:
            self.assertIn(f'qsTr("{label}")', settings + ml)
            self.assertIn(label, translations)


if __name__ == "__main__":
    unittest.main()
