"""Static integration checks for M5.4; GUI must also be built in Qt Creator on Windows."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class DesktopIntegrationContracts(unittest.TestCase):
    def test_qt_widgets_and_desktop_singleton(self):
        main = source('app/src/main.cpp')
        cmake = source('app/CMakeLists.txt')
        self.assertIn('QApplication app(argc, argv);', main)
        self.assertIn('"VirtualController.Desktop"', main)
        self.assertIn('desktopIntegration->setMainWindow(', main)
        self.assertIn('Qt6::Widgets', cmake)
        self.assertIn('DesktopIntegration.cpp', cmake)

    def test_hotkeys_remain_safe(self):
        backend = source('app/src/runtime/DesktopIntegration.cpp')
        prefs = source('app/src/runtime/AppPreferences.cpp')
        self.assertIn('RegisterHotKey(nullptr', backend)
        self.assertIn('mockMode_', backend)
        self.assertIn('UnregisterHotKey(nullptr', backend)
        self.assertIn('windows_dispatcher_MSG', backend)
        self.assertIn('runtime_->setOutputArmed(false);', backend)
        self.assertNotIn('runtime_->setOutputArmed(true)', backend)
        self.assertIn('if (key == 8) continue;', prefs)
        self.assertIn('shortcutNextProfile', source('app/qml/UiState.qml'))
        self.assertNotIn('outputArmed:', source('app/qml/UiState.qml').split('function settingsSnapshot()')[1].split('function restoreSavedSettings()')[0])

    def test_validation_block_and_non_destructive_warnings(self):
        backend = source('app/src/runtime/RuntimeController.cpp')
        mapper = source('app/src/mapping/MappingValidator.hpp')
        self.assertIn('hasBlockingMappingIssues(activeMappingProfile_)', backend)
        self.assertIn('duplicate_id', mapper)
        self.assertIn('drag_exclusive', mapper)
        self.assertIn('cursor_gate_freeze', mapper)
        self.assertIn('mappingIssues() const', backend)
        self.assertNotIn('profile.mappings.erase(', mapper)

    def test_profiles_guarded_and_ui_accessible(self):
        backend = source('app/src/runtime/RuntimeController.cpp')
        store = source('app/src/mapping/ProfileStore.cpp')
        qml = source('app/qml/pages/MappingPage.qml')
        self.assertIn('ProfileStore::validName', store)
        self.assertIn('QSaveFile', store)
        self.assertIn('duplicateActiveProfile', backend)
        self.assertIn('deleteProfile', backend)
        self.assertIn('bundled.contains', backend)
        self.assertIn('standardButtons: Dialog.Ok | Dialog.Cancel', qml)
        self.assertIn('root.uiState.mappingIssues', qml)

    def test_translations_and_original_pip_behavior(self):
        catalog = json.loads(source('app/i18n/pl.json'))
        for label in ('Duplicate profile', 'Delete profile', 'Mapping validation',
                      'Stop and disarm', 'Global shortcut unavailable: %1',
                      'Restore window'):
            self.assertTrue(catalog[label], label)
        pip = source('app/qml/components/MiniPreviewWindow.qml')
        self.assertIn('signal restoreRequested()', pip)
        self.assertNotIn('showFullScreen()', pip)


if __name__ == '__main__':
    unittest.main()
