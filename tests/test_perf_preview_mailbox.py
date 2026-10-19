"""Regression checks for the experimental 2026-10-19 preview optimization.

These source assertions do not replace a real Qt Creator / webcam test.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def content(path):
    return (ROOT / path).read_text(encoding='utf-8')


class PerformancePresentationContract(unittest.TestCase):
    def test_video_capture_is_never_driven_by_presentation_timer(self):
        cpp = content('app/src/runtime/RuntimeController.cpp')
        assert 'captureSession_.setVideoSink(&captureSink_)' in cpp
        assert 'previewMailbox_.publish(frame)' in cpp
        assert 'Qt::DirectConnection' in cpp
        assert 'previewTimer_.setTimerType(Qt::PreciseTimer)' in cpp
        assert 'activePreviewSink_->setVideoFrame(*frame)' in cpp
        assert 'previewMailbox_.clear()' in cpp
        assert 'selectPreviewTarget(' in cpp
        assert 'previewTimer_.start(timerMs)' in cpp
        assert 'timerMs = 50;' in cpp
        assert 'timerMs = 33;' in cpp

    def test_gesture_input_is_never_gated_by_presentation_rate(self):
        cpp = content('app/src/runtime/RuntimeController.cpp')
        fn = cpp.split('void RuntimeController::applyTrackingFrame(', 1)[1]
        event = fn.index('gestureStateManager_.update(gestures, eventConfig)')
        record = fn.index('recorder_.append(frame, usableGeometry)')
        submit = fn.index('updateContinuousControl(frame, events)')
        throttle = fn.index('const bool refreshUi =')
        notify = fn.index('emit stateChanged();')
        assert event < throttle
        assert record < throttle
        assert submit < throttle
        assert throttle < notify
        assert 'return;\n\n    leftLandmarks_ =' in fn

    def test_gui_queue_latency_is_visible_in_qml_and_mock(self):
        cpp = content('app/src/runtime/RuntimeController.cpp')
        header = content('app/src/runtime/RuntimeController.hpp')
        state = content('app/qml/UiState.qml')
        mock = content('app/qml/mock/UiMock.qml')
        panel = content('app/qml/components/PerformancePanel.qml')
        for name in ('guiQueueP95Ms', 'inputSubmitP95Ms'):
            assert name in header
            assert name in state
            assert name in mock
            assert name in panel
        assert 'metrics_->recordGuiQueueDelay(nowUs() - postedUs)' in cpp
        assert 'metrics_->recordInputSubmitLatency(outputSubmittedUs - frame.captureUs)' in cpp


if __name__ == '__main__':
    unittest.main()
