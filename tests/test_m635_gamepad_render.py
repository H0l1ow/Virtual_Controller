"""M6.3.5: guard against zero-width controller diagrams in Qt Layouts.

These are layout source/geometry checks; full rendered GUI verification on
Windows + Qt 6.11.2 is still necessary.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAGE = ROOT / 'app/qml/pages/GamepadPage.qml'
VISUAL = ROOT / 'app/qml/components/GamepadVisual.qml'


class GamepadLayoutRegression(unittest.TestCase):
    def setUp(self):
        self.page = PAGE.read_text(encoding='utf8')
        self.visual = VISUAL.read_text(encoding='utf8')

    def test_diagram_is_first_and_not_inside_the_old_sibling_grid(self):
        self.assertLess(self.page.index('id: diagramCard'), self.page.index('id: detailsGrid'))
        self.assertLess(self.page.index('GamepadVisual {'), self.page.index('id: detailsGrid'))
        self.assertIn('Layout.fillWidth: true\n                Layout.minimumWidth: 0\n                Layout.preferredWidth: root.pageContentWidth\n                Layout.preferredHeight: Math.max(390', self.page)
        self.assertNotIn('columns: width >= 1120 ? 2 : 1', self.page)
        self.assertNotIn('Layout.preferredWidth: width >= 1120 ? 320 : 720', self.page)
        self.assertIn('Layout.preferredWidth: root.wideLayout ? (root.pageContentWidth - 12) / 2 : root.pageContentWidth', self.page)

    def test_visual_dimensions_depend_on_actual_diagram_area(self):
        self.assertIn('width: Math.max(1, Math.min(drawingWidth, drawingHeight * 720 / 440))', self.page)
        self.assertIn('height: width * 440 / 720', self.page)
        self.assertIn('anchors.centerIn: parent', self.page)
        self.assertIn('Layout.minimumHeight: 285', self.page)
        self.assertIn('readonly property real metricTileWidth:', self.page)
        self.assertEqual(self.page.count('Layout.preferredWidth: root.metricTileWidth'), 4)
        # Simulate exact constraints from GamepadPage / 720x440 visual.
        for viewport_w, viewport_h, margin in [(500, 430, 16), (740, 520, 16),
                                               (1024, 720, 18), (1600, 900, 20)]:
            content_w = max(1, viewport_w - 2 * margin)
            card_h = max(390, min(560, viewport_h * .70))
            area_w = content_w - 24  # left/right card margins
            area_h = card_h - 50 - 1 - 34 - 7 - 5
            visual_w = max(1, min(max(1, area_w - 28), max(1, area_h - 18) * 720 / 440))
            visual_h = visual_w * 440 / 720
            self.assertGreater(visual_w, 300, (viewport_w, viewport_h))
            self.assertLessEqual(visual_w, area_w - 28)
            self.assertLessEqual(visual_h, area_h - 18)

    def test_both_layouts_render_unconditionally_without_backend(self):
        before_visual = self.page.split('GamepadVisual {')[0]
        self.assertNotIn('visible: root.mockData', before_visual)
        self.assertIn('controllerLayout: root.controllerLayout', self.page)
        self.assertIn('property bool mockData: false', self.page)
        self.assertIn('Controller layout visible even without a connected backend', self.page)
        self.assertIn('Gamepad input monitoring is not available yet.', self.page)
        self.assertIn('model: root.mockData ? root.uiState.gamepadInputs : []', self.page)
        self.assertNotIn('setGamepadBackend(', self.page)

    def test_full_button_set_remains_in_visual(self):
        for name in ('LT','LB','RT','RB','L1','L2','R1','R2',
                     'LS','RS','L3','R3','View','Home','Menu',
                     'Create','PS','Options'):
            self.assertIn('"' + name + '"', self.visual)
        for symbol in ('△', '○', '×', '□', '↑', '↓', '←', '→'):
            self.assertIn(symbol, self.visual)
        self.assertIn('dpadCanvas', self.visual)
        self.assertIn('Touchpad', self.visual)


if __name__ == '__main__':
    unittest.main()
