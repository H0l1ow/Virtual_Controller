import QtQuick
import QtQml
import QtQuick.Layouts

VcCard {
    id: root
    required property QtObject uiState
    property bool overlayMode: false

    translucent: root.overlayMode
    translucentOpacity: 0.80

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            theme: root.theme
            iconName: "settings"
            title: "Settings"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.border
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 14
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: "Sensitivity"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Text {
                    text: Math.round(sensitivitySlider.value) + "%"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }

            VcSlider {
                id: sensitivitySlider
                Layout.fillWidth: true
                theme: root.theme
                value: root.uiState.sensitivity
                onMoved: root.uiState.setSensitivity(value)
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                Text {
                    Layout.fillWidth: true
                    text: "Cursor speed"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Text {
                    text: (cursorSpeedSlider.value / 100.0).toFixed(1) + "x"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }

            VcSlider {
                id: cursorSpeedSlider
                Layout.fillWidth: true
                theme: root.theme
                from: 50
                to: 400
                stepSize: 10
                value: root.uiState.cursorSpeedPercent
                onMoved: root.uiState.setCursorSpeedPercent(value)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 2
                color: root.theme.borderSubtle
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "Invert X"
                    color: root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }
                VcSwitch {
                    id: invertXSwitch
                    theme: root.theme
                    checked: root.uiState.invertX
                    onToggled: root.uiState.setInvertX(checked)
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Invert Y"
                    color: root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }
                VcSwitch {
                    id: invertYSwitch
                    theme: root.theme
                    checked: root.uiState.invertY
                    onToggled: root.uiState.setInvertY(checked)
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Speed is a DPI-like multiplier: raise it to cross the screen with less hand travel."
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 9
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }
        }
    }

    Connections {
        target: root.uiState

        function onSensitivityChanged() {
            if (!sensitivitySlider.pressed
                    && Math.round(sensitivitySlider.value) !== root.uiState.sensitivity) {
                sensitivitySlider.value = root.uiState.sensitivity
            }
        }

        function onCursorSpeedPercentChanged() {
            if (!cursorSpeedSlider.pressed
                    && Math.round(cursorSpeedSlider.value) !== root.uiState.cursorSpeedPercent) {
                cursorSpeedSlider.value = root.uiState.cursorSpeedPercent
            }
        }

        function onInvertXChanged() {
            if (invertXSwitch.checked !== root.uiState.invertX)
                invertXSwitch.checked = root.uiState.invertX
        }

        function onInvertYChanged() {
            if (invertYSwitch.checked !== root.uiState.invertY)
                invertYSwitch.checked = root.uiState.invertY
        }
    }
}
