cmake_minimum_required(VERSION 3.24)
project(VirtualController VERSION 0.7.1 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Keep the Qt Creator / Windows baseline from the project assumptions.
set(VC_QT_MIN_VERSION "6.11.2" CACHE STRING "Minimum supported Qt 6 version")
option(VC_BUILD_GUI "Build the Qt Quick desktop application" ON)

# These switches now describe implemented functionality. Future integrations
# stay OFF until their code is actually present in the runtime.
option(VC_WITH_ONNX "Enable native ONNX gesture inference" OFF)
option(VC_WITH_GAMEPAD "Enable virtual gamepad backend" OFF)

if(DEFINED ENV{ONNXRUNTIME_ROOT} AND NOT "$ENV{ONNXRUNTIME_ROOT}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{ONNXRUNTIME_ROOT}" _vc_ort_default)
elseif(WIN32)
    set(_vc_ort_default "C:/SDK/onnxruntime-win-x64-1.26.0")
else()
    set(_vc_ort_default "${CMAKE_SOURCE_DIR}/deps/onnxruntime")
endif()
set(VC_ONNXRUNTIME_ROOT "${_vc_ort_default}" CACHE PATH "ONNX Runtime SDK root (future milestone)")

set(
    VC_DEPS_DIR
    "${CMAKE_SOURCE_DIR}/deps"
    CACHE PATH
    "Project-local dependencies"
)

set(
    VC_MEDIAPIPE_ROOT
    "${VC_DEPS_DIR}/mediapipe"
    CACHE PATH
    "MediaPipe 0.10.32 native runtime directory"
)

set(
    VC_HAND_LANDMARKER_MODEL
    "${CMAKE_SOURCE_DIR}/models/hand_landmarker.task"
    CACHE FILEPATH
    "MediaPipe hand landmarker model"
)

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR
            "VirtualController on Windows requires a Qt MSVC 2022 64-bit kit. "
            "In Qt Creator choose an MSVC x64 kit, not MinGW.")
    endif()

    if(MSVC_VERSION LESS 1930)
        message(FATAL_ERROR "Visual Studio 2022 / MSVC 19.3x or newer is required.")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "A 64-bit MSVC kit is required.")
    endif()
endif()

if(VC_BUILD_GUI)
    add_subdirectory(app)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "VirtualController ${PROJECT_VERSION} configuration:")
message(STATUS "  GUI:               ${VC_BUILD_GUI}")
message(STATUS "  Qt minimum:        ${VC_QT_MIN_VERSION}")
message(STATUS "  MediaPipe runtime: ${VC_MEDIAPIPE_ROOT}")
message(STATUS "  Hand model:        ${VC_HAND_LANDMARKER_MODEL}")
message(STATUS "  ONNX enabled:      ${VC_WITH_ONNX}")
message(STATUS "  Gamepad enabled:   ${VC_WITH_GAMEPAD}")
