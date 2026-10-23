"""M6.4.3 compatibility, safety and UI integration without Qt / a camera."""
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]


def source(name):
    return (BASE / name).read_text(encoding="utf-8")


def test_model_and_profile_compatibility_preserved():
    assert "kGestureClassCount = 5" in source("app/src/gestures/GestureTypes.hpp")
    assert "vc.mapping.profile.v2" in source("app/src/mapping/ProfileStore.cpp")
    assert "LABELS = [\"NONE\", \"FIST\", \"OPEN_HAND\", \"POINT\", \"PINCH\"]" in source("ml/vcml/schema.py")
    assert "RECORDING_LABELS" in source("ml/vcml/schema.py")


def test_experimental_classifier_is_fail_closed():
    engine = source("app/src/gestures/GestureEngine.cpp")
    manager = source("app/src/gestures/GestureStateManager.cpp")
    assert "kMinimumMargin = 0.14F" in engine
    assert "kMinimumStableUs = 120'000" in engine
    assert "rawExtraPeak >= 0.80F" in engine
    assert "prediction.experimentalMargin < kMinimumMargin" in engine
    assert "GestureDecision::Ambiguous" in engine
    assert "GestureDecision::PoseStabilizing" in manager
    assert "GestureEventPhase::Release" in manager
    assert "timestampUs - state.previousTimestampUs > kGestureMaxGapUs" in engine


def test_diagnostics_and_recording_are_connected():
    runtime = source("app/src/runtime/RuntimeController.cpp")
    page = source("app/qml/pages/GesturesPage.qml")
    recording = source("app/qml/components/RecordingCapturePanel.qml")
    assert 'QStringLiteral("experimentalMargin")' in runtime
    assert 'QStringLiteral("experimentalRunnerUp")' in runtime
    assert 'qsTr("Recognition diagnostics")' in page
    assert 'diagnosticHand.diagnostic.reason' in page
    assert 'root.experimentalRanking(diagnostic)' in page
    assert 'root.uiState.gestureCatalog' in recording
    assert 'entry.category === "static" && entry.available' in recording


def test_extended_recordings_not_silently_used_as_old_model():
    reader = source("ml/vcml/recordings.py")
    dataset = source("ml/vcml/dataset.py")
    audit = source("ml/vcml/audit.py")
    assert "RECORDING_LABELS" in reader
    assert "versioned 13-class trainer" in dataset
    assert "EXTENDED_LABELS_RECORD_ONLY" in audit
