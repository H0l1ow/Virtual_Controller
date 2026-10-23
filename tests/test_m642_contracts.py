"""M6.4.2 static landmark recognition and responsive mapping editor contracts."""
from pathlib import Path
import json
import re

BASE = Path(__file__).resolve().parents[1]


def source(rel):
    return (BASE / rel).read_text(encoding="utf-8")


def test_legacy_onnx_tensor_still_has_five_labels_and_eight_external_rules():
    types = source("app/src/gestures/GestureTypes.hpp")
    recognizer = source("app/src/gestures/ExperimentalStaticRecognizer.cpp")
    engine = source("app/src/gestures/GestureEngine.cpp")
    onnx = source("app/src/gestures/OnnxGestureRecognizer.cpp")
    assert "kGestureClassCount = 5" in types
    assert "kStaticGestureCount = 13" in types
    assert "kExperimentalStaticGestureCount" in types
    assert "experimentalStaticScores(geometry)" in engine
    assert "prediction.experimentalScores = state.experimentalSmoothed" in engine
    assert 'QStringLiteral("labels")' in onnx
    assert "kGestureClassCount" in onnx
    assert "g.pinch[0]" in recognizer
    assert "thumbDy" in recognizer


def test_all_eight_new_gestures_are_selectable_but_marked_experimental():
    registry = source("app/src/gestures/GestureRegistry.hpp")
    for label in ["THUMBS_UP", "THUMBS_DOWN", "VICTORY", "ROCK",
                  "THREE_FINGERS", "FOUR_FINGERS", "CALL_ME", "OK"]:
        assert re.search(r'\{"' + label + r'".*GestureAvailability::Experimental', registry)
    assert '"SWIPE_LEFT", "Swipe Left", GestureCategory::Dynamic, GestureAvailability::Planned' in registry
    runtime = source("app/src/runtime/RuntimeController.cpp")
    assert 'QStringLiteral("experimental")' in runtime
    assert 'entry.availability != GestureAvailability::Planned' in runtime


def test_editor_scroll_content_has_real_horizontal_insets():
    qml = source("app/qml/pages/MappingPage.qml")
    editor = qml.split("id: editPopup", 1)[1].split("id: duplicateDialog", 1)[0]
    assert "id: editorScroll" in editor
    assert "x: 20" in editor
    assert "editorScroll.availableWidth - 40" in editor
    assert "Layout.leftMargin: 20" in editor
    assert "Layout.rightMargin: 20" in editor
    assert "anchors.margins: 16" not in editor
    assert 'ScrollBar.horizontal.policy: ScrollBar.AlwaysOff' in editor


def test_mapping_filter_and_dropdown_do_not_clip_twelve_gestures():
    mapping = source("app/qml/pages/MappingPage.qml")
    combo = source("app/qml/components/VcComboBox.qml")
    assert "id: filterScroll" in mapping
    assert "Layout.fillHeight: true" in mapping
    assert 'item.category === "static" && item.available' in mapping
    assert 'result.push({code: entry.id' in mapping
    assert "Math.min(300, contentItem.implicitHeight + 2)" in combo
    assert "Math.min(296, contentHeight)" in combo


def test_gui_i18n_flags_experimental_and_tests_are_present():
    gestures = source("app/qml/pages/GesturesPage.qml")
    assert 'qsTr("Experimental")' in gestures
    assert 'catalogGesture.modelData.experimental' in gestures
    translations = json.loads(source("app/i18n/pl.json"))
    assert translations["Experimental"] == "Eksperymentalny"
    for label in ("Victory", "Three Fingers", "Four Fingers", "Rock"):
        assert translations.get(label)
    assert "ExperimentalStaticGestureTests.cpp" in source("tests/CMakeLists.txt")
    assert "ExperimentalStaticRecognizer.cpp" in source("app/CMakeLists.txt")
