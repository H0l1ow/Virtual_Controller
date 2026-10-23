"""M6.4.1 architecture contracts independent of Qt Creator and a real camera."""
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]


def source(path):
    return (BASE / path).read_text(encoding="utf-8")


def test_onnx_fixed_class_contract_untouched():
    types = source("app/src/gestures/GestureTypes.hpp")
    registry = source("app/src/gestures/GestureRegistry.hpp")
    assert "kGestureClassCount = 5" in types
    assert "kGestureRegistry" in registry
    assert '"THUMBS_UP"' in registry
    assert "GestureAvailability::Planned" in registry
    assert '"FIST"' in registry


def test_pair_arbitration_runs_before_action_mapper_and_qml_throttle():
    cpp = source("app/src/runtime/RuntimeController.cpp")
    method = cpp.split("void RuntimeController::applyTrackingFrame(", 1)[1]
    recognize = method.index("gestureStateManager_.update(gestures, eventConfig)")
    resolve = method.index("triggerCombinationEngine_.update(events)")
    dispatch = method.index("updateContinuousControl(frame, resolvedTriggers)")
    throttle = method.index("const bool refreshUi =")
    assert recognize < resolve < dispatch < throttle
    assert "actionMapper_.applyResolved(" in cpp


def test_v2_profiles_have_atomic_legacy_migration_and_safe_ui():
    store = source("app/src/mapping/ProfileStore.cpp")
    mapping = source("app/qml/pages/MappingPage.qml")
    gestures = source("app/qml/pages/GesturesPage.qml")
    assert "vc.mapping.profile.v1" in store
    assert "vc.mapping.profile.v2" in store
    assert ".v1.bak" in store
    assert "QSaveFile" in store
    assert "root.uiState.updateTriggerMapping(" in mapping
    assert "Gesture sequence (planned)" in mapping
    assert "root.uiState.gestureCatalog" in gestures
    assert "catalogGesture.modelData.available" in gestures
