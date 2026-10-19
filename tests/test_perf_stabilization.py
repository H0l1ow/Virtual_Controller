"""M6.2.2 performance stabilization: source contracts, not Windows GUI tests."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return (ROOT / name).read_text(encoding="utf-8")


class PerformanceStabilization(unittest.TestCase):
    def test_processed_gui_results_bounded_before_inference(self):
        source = read("app/src/runtime/RuntimeController.cpp")
        worker = source.split("void RuntimeController::workerLoop(", 1)[1]
        wait = worker.index("guiGate->acquire(")
        inference = worker.index("tracker.process(")
        queued = worker.index("QMetaObject::invokeMethod(", inference)
        self.assertLess(wait, inference)
        self.assertLess(inference, queued)
        self.assertIn("[this, tracking, gestures, generation, postedUs, guiLease]", worker)
        self.assertIn('throw std::runtime_error("Cannot queue tracking result to GUI")', worker)

    def test_gestures_mapped_without_ui_throttle(self):
        cpp = read("app/src/runtime/RuntimeController.cpp")
        method = cpp.split("void RuntimeController::applyTrackingFrame(", 1)[1]
        self.assertLess(method.index("gestureStateManager_.update("),
                        method.index("const bool refreshUi ="))
        self.assertLess(method.index("updateContinuousControl(frame, events)"),
                        method.index("const bool refreshUi ="))
        self.assertIn("outputService_.stop();", cpp)

    def test_async_recording_flushes_on_pipeline_stop(self):
        cpp = read("app/src/runtime/RuntimeController.cpp")
        body = cpp.split("void RuntimeController::requestSessionStop(", 1)[1]
        self.assertIn("if (recorder_.active())\n        stopGestureRecording();", body)
        recorder = read("app/src/gestures/GestureDataRecorder.cpp")
        self.assertIn("writer_->enqueue(line.str())", recorder)
        self.assertIn("writer_->close()", recorder)
        writer = read("app/src/gestures/AsyncJsonlWriter.hpp")
        self.assertIn("kMaxQueuedLines = 256", writer)
        self.assertIn("std::thread thread_", writer)

    def test_preview_resources_released_while_capture_independent(self):
        cpp = read("app/src/runtime/RuntimeController.cpp")
        self.assertIn("captureSession_.setVideoSink(&captureSink_)", cpp)
        self.assertIn("previewMailbox_.publish(frame)", cpp)
        self.assertIn("if (pipSink_) pipSink_->setVideoFrame(QVideoFrame{});", cpp)
        self.assertIn("if (recordingSink_) recordingSink_->setVideoFrame(QVideoFrame{});", cpp)

    def test_latency_metric_is_end_to_end_and_localized(self):
        files = (
            "app/src/runtime/RuntimeMetrics.hpp",
            "app/src/runtime/RuntimeController.hpp",
            "app/qml/UiState.qml",
            "app/qml/mock/UiMock.qml",
            "app/qml/components/PerformancePanel.qml",
        )
        for path in files:
            self.assertIn("guiBackpressureP95Ms", read(path))
        self.assertIn('"Worker backpressure (P95)"', read("app/i18n/pl.json"))


if __name__ == "__main__":
    unittest.main()
