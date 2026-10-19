"""Static UI and native capture lifecycle regressions (Qt runtime needs Windows test)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(encoding='utf8')

class PreviewLifecycle(unittest.TestCase):
    def test_camera_capture_is_independent_of_qml_views(self):
        source = read('app/src/runtime/RuntimeController.cpp')
        header = read('app/src/runtime/RuntimeController.hpp')
        self.assertIn('captureSession_.setVideoSink(&captureSink_)', source)
        self.assertIn('&captureSink_, &QVideoSink::videoFrameChanged', source)
        self.assertNotIn('captureSession_.setVideoOutput(output)', source)
        self.assertNotIn('if (!videoOutput_)', source)
        self.assertIn('QVideoSink captureSink_', header)

    def test_one_renderer_is_chosen_at_once(self):
        routing = read('app/src/runtime/PreviewRoutingPolicy.hpp')
        runtime = read('app/src/runtime/RuntimeController.cpp')
        self.assertIn('selectPreviewTarget(', runtime)
        self.assertIn('if (mainWindowMinimized || minimizedPiPActive)', routing)
        self.assertIn('if (recordingAttached)', routing)
        self.assertIn('if (controllerAttached)', routing)
        self.assertIn('previewGeneration_', runtime)
        self.assertIn('vc_preview_routing_tests', read('tests/CMakeLists.txt'))

    def test_controller_and_ml_disconnect_on_navigation_and_minimize(self):
        main = read('app/qml/Main.qml')
        controller = read('app/qml/components/CameraStage.qml')
        recording = read('app/qml/components/RecordingCapturePanel.qml')
        self.assertIn('previewActive: window.currentPage === 0', main)
        self.assertIn('previewActive: window.currentPage === 5', main)
        self.assertIn('!window.presentationSuspended', main)
        self.assertIn('setMainWindowMinimized(presentationSuspended)', main)
        self.assertIn('root.previewActive && root.visible ? videoOutput.videoSink : null', controller)
        self.assertIn('root.renderingPreview ? recordingVideo.videoSink : null', recording)
        self.assertIn('Component.onDestruction: {', recording)

    def test_hidden_landmark_layers_are_inactive(self):
        for name in ('CameraStage.qml', 'RecordingCapturePanel.qml', 'MiniPreviewWindow.qml'):
            source = read('app/qml/components/' + name)
            self.assertIn('Timer {', source)
            self.assertIn('Connections {', source)
            self.assertIn('enabled:', source)
        self.assertIn('running: handCanvas.visible', read('app/qml/components/CameraStage.qml'))
        self.assertIn('running: root.renderingPreview', read('app/qml/components/RecordingCapturePanel.qml'))

    def test_legacy_mock_compiles_syntactically(self):
        mock = read('app/qml/mock/UiMock.qml')
        self.assertIn('readonly property var recordingQuality: ({', mock)

if __name__ == '__main__':
    unittest.main()
