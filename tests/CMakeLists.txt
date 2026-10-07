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
