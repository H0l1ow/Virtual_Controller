"""M5.2.1 regression contracts: Polish toolbar and PiP localization (legacy restore behavior).

Static checks only; Windows Qt 6.11.2 runtime smoke testing is still required.
"""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def qml(path):
    return (ROOT / 'app' / 'qml' / path).read_text(encoding='utf-8')


class UiHotfixTests(unittest.TestCase):
    def test_mapping_toolbar_buttons_are_content_sized(self):
        mapping = qml('pages/MappingPage.qml')
        for source in ('Reload', 'Add mapping', 'Remove'):
            pattern = r'text: qsTr\("' + re.escape(source) + r'"\)(?:(?!\n\s*}).)*'
            match = re.search(pattern, mapping, flags=re.S)
            self.assertIsNotNone(match, source)
            self.assertNotIn('implicitWidth:', match.group(0), source)
        button = qml('components/VcButton.qml')
        self.assertIn('labelText.implicitWidth', button)
        self.assertIn('elide: Text.ElideRight', button)
        toolbar = qml('components/PageToolbar.qml')
        self.assertIn('Layout.minimumWidth: 0', toolbar)
        self.assertGreaterEqual(toolbar.count('elide: Text.ElideRight'), 2)

    def test_pip_strings_are_localized_and_gestures_translated(self):
        mini = qml('components/MiniPreviewWindow.qml')
        catalog = json.loads((ROOT / 'app/i18n/pl.json').read_text(encoding='utf-8'))
        self.assertIn('import VirtualController.I18n 1.0', mini)
        self.assertIn('LanguageManager.language', mini)
        for label in ('Virtual Controller - Mini preview', 'Live preview', 'Camera inactive', 'Left', 'Right', 'PINCH', 'FIST', 'OPEN HAND', 'POINT', 'NONE'):
            self.assertTrue(catalog.get(label), label)
        self.assertIn('mini.uiText(mini.uiState.leftGesture', mini)
        self.assertIn('mini.uiText(mini.uiState.rightGesture', mini)
        self.assertNotIn('text: "L: "', mini)

    def test_pip_keeps_original_m52_restore_behavior(self):
        mini = qml('components/MiniPreviewWindow.qml')
        main = qml('Main.qml')
        self.assertIn('onPressed: mini.startSystemMove()', mini)
        self.assertIn('onDoubleClicked: mini.restoreRequested()', mini)
        self.assertIn('onClicked: mini.restoreRequested()', mini)
        self.assertEqual(mini.count('mini.restoreRequested()'), 2)
        self.assertIn('if (controllerFullscreen)', main)
        self.assertIn('showNormal()', main)
        self.assertNotIn('Clicking PiP enters the real fullscreen presentation mode', main)
        self.assertIn('onRestoreRequested: window.restoreMainWindow()', main)
        self.assertIn('onClicked: mini.dismissed = true', mini)


if __name__ == '__main__':
    unittest.main()
