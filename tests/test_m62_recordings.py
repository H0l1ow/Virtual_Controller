"""M6.2 static integration guardrails (full Qt/MSVC GUI still needs Windows validation)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

def read(p):
    return (ROOT / p).read_text(encoding='utf-8')


class RecordingIntegration(unittest.TestCase):
    def test_ml_page_only_reorganizes_existing_style(self):
        page = read('app/qml/pages/MLPage.qml')
        self.assertIn('RecordingCapturePanel {', page)
        self.assertIn('RecordingFilesPanel {', page)
        self.assertIn('Reload model', page)
        self.assertIn('Gesture Replay (offline)', page)
        self.assertIn('Offline training & evaluation', page)

    def test_preview_uses_secondary_sink_and_no_new_camera(self):
        ui = read('app/qml/components/RecordingCapturePanel.qml')
        runtime = read('app/src/runtime/RuntimeController.cpp')
        self.assertIn('attachRecordingVideoSink(', ui)
        self.assertIn('recordingVideo.videoSink', ui)
        self.assertIn('root.uiState.leftLandmarks', ui)
        self.assertIn('QVideoSink::videoFrameChanged', runtime)
        self.assertIn('updatePreviewConnections()', runtime)
        self.assertIn('captureSession_.setVideoSink(&captureSink_)', runtime)
        self.assertNotIn('QCamera {', ui)

    def test_files_restricted_to_own_dir_and_trash(self):
        code = read('app/src/runtime/RuntimeController.cpp')
        self.assertIn('QStandardPaths::AppLocalDataLocation', code)
        self.assertIn('isSymLink()', code)
        self.assertIn('file.moveToTrash()', code)
        self.assertIn('recording/directory', code)
        self.assertIn('stopGestureReplay', code)

    def test_progress_warns_not_accuracy(self):
        ui = read('app/qml/components/RecordingCapturePanel.qml')
        for text in ('full bar does not validate', 'Missing/invalid',
                     'at least one hand', 'Record samples'):
            self.assertIn(text, ui)

    def test_public_ui_contract_and_mock(self):
        backend = read('app/src/runtime/RuntimeController.hpp')
        uistate = read('app/qml/UiState.qml')
        mock = read('app/qml/mock/UiMock.qml')
        for name in ('recordingQuality', 'recordingsDirectory', 'recordings',
                     'recordingsError', 'deleteRecordingFile', 'openRecordingFile'):
            self.assertIn(name, backend)
            self.assertIn(name, uistate)
            self.assertIn(name, mock)

if __name__ == '__main__':
    unittest.main()
