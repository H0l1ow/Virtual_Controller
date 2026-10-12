"""Pure-stdlib localization regression checks; no Qt installation required."""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / "app" / "qml"
CATALOG = ROOT / "app" / "i18n" / "pl.json"
TRANSLATABLE = re.compile(r'qsTr\("((?:\\.|[^"\\])*)"\)')


class LocalizationTests(unittest.TestCase):
    def setUp(self):
        self.catalog = json.loads(CATALOG.read_text(encoding="utf-8"))

    def test_every_static_translation_has_a_polish_value(self):
        missing = []
        for path in QML.rglob("*.qml"):
            for escaped in TRANSLATABLE.findall(path.read_text(encoding="utf-8")):
                source = json.loads('"' + escaped + '"')
                if not self.catalog.get(source, "").strip():
                    missing.append(f"{path.relative_to(ROOT)}: {source!r}")
        self.assertEqual(missing, [])

    def test_catalog_is_utf8_and_nonempty(self):
        self.assertGreater(len(self.catalog), 350)
        self.assertEqual(self.catalog["Settings"], "Ustawienia")
        self.assertEqual(self.catalog["Language"], "Język")
        self.assertEqual(self.catalog["Stop tracking"], "Zatrzymaj śledzenie")

    def test_profile_protocol_values_are_never_localized(self):
        mapping = (QML / "pages" / "MappingPage.qml").read_text(encoding="utf-8")
        for option in ("Left", "Right", "Press", "Hold", "Toggle", "Left click", "PINCH"):
            self.assertIn('"' + option + '"', mapping)
        self.assertIn("labelFormatter: root.displayOption", mapping)
        self.assertIn("handCombo.currentText", mapping)
        self.assertIn("actionCombo.currentText", mapping)
        test_page = (QML / "pages" / "TestPage.qml").read_text(encoding="utf-8")
        self.assertIn('exercise === "Click targets"', test_page)
        self.assertNotIn('exercise === qsTr("Click targets")', test_page)

    def test_selector_persistence_and_qml_retranslate(self):
        manager = (ROOT / "app" / "src" / "i18n" / "LanguageManager.cpp").read_text(encoding="utf-8")
        settings = (QML / "pages" / "SettingsPage.qml").read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("ui/language")', manager)
        self.assertIn("m_engine->retranslate()", manager)
        self.assertIn("languageManager.setLanguageByIndex(index)", settings)
        self.assertIn("currentIndex: languageManager.languageIndex", settings)


if __name__ == "__main__":
    unittest.main()
