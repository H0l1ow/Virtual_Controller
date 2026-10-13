add_executable(
    vc_hand_identity_tests
    HandIdentityStabilizerTests.cpp
    ../app/src/tracking/HandIdentityStabilizer.cpp
)

target_include_directories(
    vc_hand_identity_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_hand_identity_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_hand_identity_tests
    COMMAND vc_hand_identity_tests
)

add_executable(
    vc_runtime_metrics_tests
    RuntimeMetricsTests.cpp
    ../app/src/runtime/RuntimeMetrics.cpp
)

target_include_directories(
    vc_runtime_metrics_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_runtime_metrics_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_runtime_metrics_tests
    COMMAND vc_runtime_metrics_tests
)

add_executable(
    vc_continuous_control_tests
    ContinuousControlTests.cpp
    ../app/src/control/OneEuroFilter.cpp
    ../app/src/control/ContinuousControlInterpreter.cpp
)

target_include_directories(
    vc_continuous_control_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_continuous_control_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_continuous_control_tests
    COMMAND vc_continuous_control_tests
)

add_executable(
    vc_output_guard_tests
    OutputGuardTests.cpp
    ../app/src/input/OutputGuard.cpp
)

target_include_directories(
    vc_output_guard_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_output_guard_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_output_guard_tests
    COMMAND vc_output_guard_tests
)

add_executable(
    vc_output_service_tests
    OutputServiceTests.cpp
    ../app/src/input/InputBackend.cpp
    ../app/src/input/OutputGuard.cpp
    ../app/src/input/OutputService.cpp
)

target_include_directories(
    vc_output_service_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_output_service_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_output_service_tests
    COMMAND vc_output_service_tests
)

add_executable(
    vc_gesture_recognition_tests
    GestureRecognitionTests.cpp
    ../app/src/gestures/HandFeatures.cpp
    ../app/src/gestures/GestureEngine.cpp
)

target_include_directories(
    vc_gesture_recognition_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_gesture_recognition_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_gesture_recognition_tests
    COMMAND vc_gesture_recognition_tests
)

add_executable(
    vc_gesture_state_tests
    GestureStateManagerTests.cpp
    ../app/src/gestures/GestureStateManager.cpp
)

target_include_directories(
    vc_gesture_state_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_gesture_state_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_gesture_state_tests
    COMMAND vc_gesture_state_tests
)

add_executable(
    vc_action_mapper_tests
    ActionMapperTests.cpp
    ../app/src/mapping/ActionMapper.cpp
)

target_include_directories(
    vc_action_mapper_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
)

target_compile_features(
    vc_action_mapper_tests
    PRIVATE cxx_std_20
)

add_test(
    NAME vc_action_mapper_tests
    COMMAND vc_action_mapper_tests
)

# M5 JSONL recorder is std::filesystem-only and runs without Qt/camera.
add_executable(vc_gesture_recorder_tests
    GestureDataRecorderTests.cpp
    ../app/src/gestures/GestureDataRecorder.cpp
)
target_include_directories(vc_gesture_recorder_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/src)
target_compile_features(vc_gesture_recorder_tests PRIVATE cxx_std_20)
add_test(NAME vc_gesture_recorder_tests COMMAND vc_gesture_recorder_tests)

# M5.2.2: Qt-independent camera unplug/replug recovery policy.
add_executable(vc_camera_reconnect_tests CameraReconnectPolicyTests.cpp)
target_include_directories(vc_camera_reconnect_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/src)
target_compile_features(vc_camera_reconnect_tests PRIVATE cxx_std_20)
add_test(NAME vc_camera_reconnect_tests COMMAND vc_camera_reconnect_tests)

# M5.3: data-independent calibration recommendations and isolated pointer test.
add_executable(vc_gesture_calibration_tests
    GestureCalibratorTests.cpp
    ../app/src/gestures/GestureCalibrator.cpp
)
target_include_directories(vc_gesture_calibration_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/src)
target_compile_features(vc_gesture_calibration_tests PRIVATE cxx_std_20)
add_test(NAME vc_gesture_calibration_tests COMMAND vc_gesture_calibration_tests)

add_executable(vc_cursor_playground_tests
    CursorPlaygroundTests.cpp
    ../app/src/control/CursorPlayground.cpp
)
target_include_directories(vc_cursor_playground_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/src)
target_compile_features(vc_cursor_playground_tests PRIVATE cxx_std_20)
add_test(NAME vc_cursor_playground_tests COMMAND vc_cursor_playground_tests)

# M5.4 profile conflict detection, independent of Qt and Windows.
add_executable(vc_mapping_validator_tests MappingValidatorTests.cpp)
target_include_directories(vc_mapping_validator_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/src)
target_compile_features(vc_mapping_validator_tests PRIVATE cxx_std_20)
add_test(NAME vc_mapping_validator_tests COMMAND vc_mapping_validator_tests)
