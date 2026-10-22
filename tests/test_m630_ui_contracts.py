"""UI architecture contract tests: static checks, not a substitute for running Qt on Windows."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QML = ROOT / 'app/qml'
def source(path):
    return (ROOT / path).read_text(encoding='utf8')

class M630UiRefresh(unittest.TestCase):
    def test_navigates_seven_existing_pages_without_changing_runtime(self):
        main=source('app/qml/Main.qml')
        sidebar=source('app/qml/components/AppSideBar.qml')
        self.assertEqual(len(re.findall(r'\b(?:Controller|Mapping|Gestures|Test|Gamepad|ML|Settings)Page\s*\{',main)),7)
        self.assertIn('collapsed: window.sidebarCollapsed', main)
        self.assertIn('onPageSelected: window.selectPage(index)',main)
        self.assertIn('visible: !window.controllerFullscreen',main)
        self.assertIn('minimizeActive: window.presentationInitialized && window.presentationSuspended',main)
        self.assertIn('page: 5,',sidebar)
        self.assertIn('page: 6,',sidebar)
    def test_camera_and_output_safety_bindings_survive(self):
        controller=source('app/qml/pages/ControllerPage.qml')
        camera=source('app/qml/components/CameraStage.qml')
        side=source('app/qml/components/DashboardSidePanel.qml')
        self.assertIn('previewActive: root.previewActive && !root.immersiveMode',controller)
        self.assertIn('previewActive: root.previewActive && root.immersiveMode',controller)
        self.assertIn('root.uiState.attachVideoOutput(',camera)
        self.assertIn('root.uiState.setOutputArmed(checked)',side)  # switch moved inside camera overlay
        self.assertIn('root.uiState.togglePipeline()',controller)
        self.assertIn('root.uiState.outputArmed',side)
        self.assertIn('root.uiState.outputActive',side)
    def test_advanced_view_is_lazy_not_a_second_camera(self):
        disclosure=source('app/qml/components/DisclosurePanel.qml')
        self.assertIn('active: root.expanded',disclosure)
        self.assertIn('Behavior on implicitHeight',disclosure)
        self.assertNotIn('VideoOutput',disclosure)
        self.assertIn('DashboardSidePanel {',source('app/qml/components/CameraStage.qml'))
    def test_recording_and_backend_are_preserved(self):
        ml=source('app/qml/pages/MLPage.qml')
        self.assertLess(ml.index('RecordingCapturePanel {'), ml.index('RecordingFilesPanel {'))
        self.assertLess(ml.index('RecordingFilesPanel {'), ml.index('Recognition model'))
        self.assertIn('RecordingFilesPanel {',ml)
        self.assertIn('DisclosurePanel {',ml)
        self.assertIn('reloadGestureModel()',ml)
        self.assertIn('startGestureReplay(selectedFile.toString())',ml)
    def test_inert_controls_and_unused_nav_removed(self):
        self.assertFalse((QML/'components/NavTab.qml').exists())
        self.assertNotIn('NavTab.qml',source('app/CMakeLists.txt'))
        self.assertNotIn('enabled: false',source('app/qml/components/ControlsCard.qml'))
        self.assertNotIn('enabled: false',source('app/qml/pages/GamepadPage.qml'))
        self.assertNotIn('property string outputMode:',source('app/qml/UiState.qml'))
        self.assertTrue((ROOT/'docs/releases/PATCH_M6_2_3_README.md').is_file())
    def test_settings_navigation_uses_categories(self):
        settings=source('app/qml/pages/SettingsPage.qml')
        self.assertIn('id: sectionItem',settings)
        self.assertIn('root.sectionIndex = sectionItem.index',settings)
        self.assertIn('LanguageManager.setLanguageByIndex(index)',settings)
        self.assertIn('root.uiState.saveSettings()',settings)
    def test_mapping_actions_remain_available(self):
        mapping=source('app/qml/pages/MappingPage.qml')
        self.assertIn('id: mappingActions',mapping)
        self.assertIn('root.uiState.reloadActiveProfile()',mapping)
        self.assertIn('root.uiState.removeMapping(index)',mapping)
        self.assertIn('root.uiState.addMapping()',mapping)

if __name__ == '__main__': unittest.main()
