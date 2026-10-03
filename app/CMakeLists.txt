find_package(Qt6 ${VC_QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
qt_standard_project_setup()

message(STATUS "Using Qt ${Qt6_VERSION} from ${Qt6_DIR}")

qt_add_executable(VirtualController
    src/main.cpp
)

target_link_libraries(VirtualController PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

if(MSVC)
    target_compile_options(VirtualController PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
endif()

qt_add_resources(VirtualController "qml"
    PREFIX "/"
    FILES
        qml/Main.qml
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
        qml/components/WindowControlButton.qml

        qml/pages/ControllerPage.qml
        qml/pages/MappingPage.qml
        qml/pages/GesturesPage.qml
        qml/pages/GamepadPage.qml
        qml/pages/SettingsPage.qml
)

set_target_properties(VirtualController PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS VirtualController RUNTIME DESTINATION . BUNDLE DESTINATION .)
