"""Static integration contracts for M5.2, runnable without the Qt SDK.

These complement, but do not replace, a Windows GUI/QCamera smoke test.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class SettingsAndPiPContracts(unittest.TestCase):
    def test_persistent_settings_registered_in_qt_creator(self):
        app = source("app/CMakeLists.txt")
        main = source("app/src/main.cpp")
        header = source("app/src/runtime/AppPreferences.hpp")
        impl = source("app/src/runtime/AppPreferences.cpp")
        for name in ("AppPreferences.cpp", "AppPreferences.hpp", "MiniPreviewWindow.qml"):
            self.assertIn(name, app)
        self.assertIn('"VirtualController.Preferences"', main)
        self.assertIn('"AppPreferences"', main)
        self.assertIn("QSettings settings", impl)
        self.assertIn("settings.sync()", impl)
        self.assertIn("QSettings::NoError", impl)
        self.assertIn("effectiveThemeChanged", header)
        self.assertIn("colorSchemeChanged", impl)

    def test_save_is_manual_and_never_rearms_input(self):
        ui = source("app/qml/UiState.qml")
        settings = source("app/qml/pages/SettingsPage.qml")
        self.assertIn("hasUnsavedSettings", ui)
        self.assertIn("AppPreferences.saveSettings(snapshot)", ui)
        self.assertIn("setOutputArmed(false)", ui)
        self.assertNotIn("outputArmed: outputArmed", ui)
        self.assertIn("enabled: root.uiState.hasUnsavedSettings", settings)
        self.assertIn("onClicked: root.uiState.saveSettings()", settings)

    def test_pip_reuses_camera_sink(self):
        cpp = source("app/src/runtime/RuntimeController.cpp")
        window = source("app/qml/components/MiniPreviewWindow.qml")
        main = source("app/qml/Main.qml")
        self.assertIn("captureSession_.setVideoSink(&captureSink_)", cpp)
        self.assertIn("pipSink_", cpp)
        self.assertIn("activePreviewSink_->setVideoFrame(*frame)", cpp)
        self.assertIn("videoFrameChanged", cpp)
        self.assertNotIn("QCamera {", window)
        self.assertIn("attachPiPVideoSink(videoSink)", window)
        self.assertIn("WindowStaysOnTopHint", window)
        self.assertIn("setPiPActive(visible)", window)
        self.assertIn("window.visibility === Window.Minimized", main)

    def test_theme_and_translation(self):
        theme = source("app/qml/theme/Theme.qml")
        settings = source("app/qml/pages/SettingsPage.qml")
        self.assertIn('isLight ? "#F3F6F8"', theme)
        self.assertIn("AppPreferences.effectiveTheme", source("app/qml/Main.qml"))
        self.assertIn('qsTr("Mini camera preview")', settings)
        self.assertIn('qsTr("Follow Windows")', settings)
        self.assertNotIn('implicitWidth: 110\n                                    enabled: false', settings)


if __name__ == "__main__":
    unittest.main()
