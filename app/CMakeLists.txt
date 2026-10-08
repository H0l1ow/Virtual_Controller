find_package(
    Qt6 ${VC_QT_MIN_VERSION}
    REQUIRED COMPONENTS
        Core
        Gui
        Qml
        Quick
        QuickControls2
        Multimedia
)

qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

message(STATUS "Using Qt ${Qt6_VERSION} from ${Qt6_DIR}")

qt_add_executable(
    VirtualController

    src/main.cpp

    src/runtime/LatestFrameSlot.hpp
    src/runtime/LatestFrameSlot.cpp
    src/runtime/RuntimeMetrics.hpp
    src/runtime/RuntimeMetrics.cpp
    src/runtime/RuntimeController.hpp
    src/runtime/RuntimeController.cpp

    src/control/ControllerState.hpp
    src/control/OneEuroFilter.hpp
    src/control/OneEuroFilter.cpp
    src/control/ContinuousControlInterpreter.hpp
    src/control/ContinuousControlInterpreter.cpp

    src/gestures/GestureTypes.hpp
    src/gestures/HandFeatures.hpp
    src/gestures/HandFeatures.cpp
    src/gestures/GestureEngine.hpp
    src/gestures/GestureEngine.cpp
    src/gestures/GestureStateManager.hpp
    src/gestures/GestureStateManager.cpp
    src/gestures/OnnxGestureRecognizer.hpp
    src/gestures/OnnxGestureRecognizer.cpp

    src/input/InputBackend.hpp
    src/input/InputBackend.cpp
    src/input/OutputGuard.hpp
    src/input/OutputGuard.cpp
    src/input/OutputService.hpp
    src/input/OutputService.cpp

    src/mapping/MappingTypes.hpp
    src/mapping/ActionMapper.hpp
    src/mapping/ActionMapper.cpp
    src/mapping/ProfileStore.hpp
    src/mapping/ProfileStore.cpp

    src/tracking/TrackingTypes.hpp
    src/tracking/HandIdentityStabilizer.hpp
    src/tracking/HandIdentityStabilizer.cpp
    src/tracking/MediaPipeAbi.hpp
    src/tracking/MediaPipeTracker.hpp
    src/tracking/MediaPipeTracker.cpp
)

target_include_directories(
    VirtualController
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    VirtualController
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Multimedia
        Threads::Threads
)

target_compile_definitions(
    VirtualController
    PRIVATE
        VC_APP_VERSION="${PROJECT_VERSION}"
        VC_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
        VC_MEDIAPIPE_ROOT_PATH="${VC_MEDIAPIPE_ROOT}"
        VC_HAND_LANDMARKER_MODEL_PATH="${VC_HAND_LANDMARKER_MODEL}"
        VC_GESTURE_MODEL_PATH="${VC_GESTURE_MODEL}"
)

if(VC_WITH_ONNX)
    target_compile_definitions(
        VirtualController
        PRIVATE VC_WITH_ONNX=1
    )

    target_include_directories(
        VirtualController
        PRIVATE "${VC_ONNXRUNTIME_INCLUDE_DIR}"
    )

    target_link_libraries(
        VirtualController
        PRIVATE "${VC_ONNXRUNTIME_LIBRARY}"
    )
endif()

if(MSVC)
    target_compile_options(
        VirtualController
        PRIVATE
            /W4
            /permissive-
            /utf-8
            /Zc:__cplusplus
    )
endif()

qt_add_resources(
    VirtualController
    "qml"
    PREFIX "/"
    FILES
        qml/Main.qml
        qml/UiState.qml

        qml/catalog/UiCatalog.qml
        qml/theme/Theme.qml
        qml/mock/UiMock.qml

        qml/components/AppTopBar.qml
        qml/components/CameraStage.qml
        qml/components/ControlsCard.qml
        qml/components/CursorTrackingCard.qml
        qml/components/GamepadVisual.qml
        qml/components/GestureGlyph.qml
        qml/components/GestureHintRow.qml
        qml/components/HandLogo.qml
        qml/components/MetricTile.qml
        qml/components/NavTab.qml
        qml/components/PageToolbar.qml
        qml/components/PanelHeader.qml
        qml/components/QuickSettingsCard.qml
        qml/components/SettingsRow.qml
        qml/components/StatusBadge.qml
        qml/components/TagChip.qml
        qml/components/VcButton.qml
        qml/components/VcCard.qml
        qml/components/VcComboBox.qml
        qml/components/VcIcon.qml
        qml/components/VcIconButton.qml
        qml/components/VcSlider.qml
        qml/components/VcSwitch.qml
        qml/components/VcTextField.qml
        qml/components/ViewOptionsPanel.qml
        qml/components/WindowControlButton.qml

        qml/pages/ControllerPage.qml
        qml/pages/MappingPage.qml
        qml/pages/GesturesPage.qml
        qml/pages/TestPage.qml
        qml/pages/GamepadPage.qml
        qml/pages/SettingsPage.qml
)

# Keep Qt Creator Build/Run self-contained. The runtime also has source-tree
# fallbacks, so a missing optional copy does not prevent configuration.
if(WIN32 AND EXISTS "${VC_MEDIAPIPE_ROOT}/libmediapipe.dll")
    add_custom_command(
        TARGET VirtualController
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VirtualController>/native"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VC_MEDIAPIPE_ROOT}/libmediapipe.dll"
            "$<TARGET_FILE_DIR:VirtualController>/native/libmediapipe.dll"
        VERBATIM
    )
endif()

if(EXISTS "${VC_HAND_LANDMARKER_MODEL}")
    add_custom_command(
        TARGET VirtualController
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VirtualController>/models"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VC_HAND_LANDMARKER_MODEL}"
            "$<TARGET_FILE_DIR:VirtualController>/models/hand_landmarker.task"
        VERBATIM
    )
endif()

if(EXISTS "${VC_GESTURE_MODEL}")
    add_custom_command(
        TARGET VirtualController
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:VirtualController>/models"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VC_GESTURE_MODEL}"
            "$<TARGET_FILE_DIR:VirtualController>/models/gesture_model.onnx"
        VERBATIM
    )

    get_filename_component(_vc_gesture_model_dir "${VC_GESTURE_MODEL}" DIRECTORY)
    if(EXISTS "${_vc_gesture_model_dir}/model_metadata.json")
        add_custom_command(
            TARGET VirtualController
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_vc_gesture_model_dir}/model_metadata.json"
                "$<TARGET_FILE_DIR:VirtualController>/models/model_metadata.json"
            VERBATIM
        )
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/profiles")
    add_custom_command(
        TARGET VirtualController
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/profiles"
            "$<TARGET_FILE_DIR:VirtualController>/profiles"
        VERBATIM
    )
endif()

if(WIN32 AND VC_WITH_ONNX)
    set(_vc_ort_dll "${VC_ONNXRUNTIME_ROOT}/lib/onnxruntime.dll")
    if(NOT EXISTS "${_vc_ort_dll}")
        set(_vc_ort_dll "${VC_ONNXRUNTIME_ROOT}/bin/onnxruntime.dll")
    endif()

    if(EXISTS "${_vc_ort_dll}")
        add_custom_command(
            TARGET VirtualController
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_vc_ort_dll}"
                "$<TARGET_FILE_DIR:VirtualController>/onnxruntime.dll"
            VERBATIM
        )
    endif()
endif()

set_target_properties(
    VirtualController
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

install(
    TARGETS VirtualController
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)

if(WIN32 AND EXISTS "${VC_MEDIAPIPE_ROOT}/libmediapipe.dll")
    install(
        FILES "${VC_MEDIAPIPE_ROOT}/libmediapipe.dll"
        DESTINATION native
    )
endif()

if(EXISTS "${VC_HAND_LANDMARKER_MODEL}")
    install(
        FILES "${VC_HAND_LANDMARKER_MODEL}"
        DESTINATION models
    )
endif()

if(EXISTS "${VC_GESTURE_MODEL}")
    install(
        FILES "${VC_GESTURE_MODEL}"
        DESTINATION models
    )

    get_filename_component(_vc_gesture_model_dir "${VC_GESTURE_MODEL}" DIRECTORY)
    if(EXISTS "${_vc_gesture_model_dir}/model_metadata.json")
        install(
            FILES "${_vc_gesture_model_dir}/model_metadata.json"
            DESTINATION models
        )
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/profiles")
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/profiles/"
        DESTINATION profiles
        FILES_MATCHING PATTERN "*.json" PATTERN "README.md"
    )
endif()
