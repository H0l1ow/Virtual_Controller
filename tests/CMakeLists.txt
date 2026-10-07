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
