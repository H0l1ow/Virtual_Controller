import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Gamepad Monitor"
            subtitle: "Live logical controller state. The virtual device backend will be connected in a later stage."

            VcComboBox {
                theme: root.theme
                model: ["Xbox layout", "PlayStation layout (planned)"]
                currentIndex: 0
                implicitWidth: 190
                enabled: false
            }

            StatusBadge {
                theme: root.theme
                text: "Mock backend"
                kind: "warning"
            }

            StatusBadge {
                theme: root.theme
                text: root.uiState.outputArmed ? "Output armed" : "Output disarmed"
                kind: root.uiState.outputArmed ? "success" : "neutral"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 620
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        showIcon: false
                        title: "Controller State"
                        statusText: root.uiState.pipelineRunning ? "Live" : "Stopped"
                        statusKind: root.uiState.pipelineRunning ? "success" : "neutral"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    Item {
                        id: gamepadArea

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 300
                        Layout.leftMargin: 18
                        Layout.rightMargin: 18
                        Layout.topMargin: 10
                        Layout.bottomMargin: 10
                        clip: true

                        GamepadVisual {
                            id: gamepadVisual

                            anchors.centerIn: parent
                            theme: root.theme

                            readonly property real availableWidth: Math.max(0, gamepadArea.width - 20)
                            readonly property real availableHeight: Math.max(0, gamepadArea.height - 20)
                            readonly property real targetWidth: Math.min(
                                availableWidth,
                                availableHeight * 720 / 440
                            )

                            width: targetWidth
                            height: width * 440 / 720
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        Layout.preferredHeight: 58
                        spacing: 8

                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "target"
                            value: root.uiState.leftStickValue
                            label: "Left Stick X / Y"
                        }

                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "target"
                            value: root.uiState.rightStickValue
                            label: "Right Stick X / Y"
                        }

                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "play"
                            value: root.uiState.triggerValue
                            label: "LT / RT"
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 320
                Layout.minimumWidth: 300
                Layout.maximumWidth: 360
                Layout.fillHeight: true
                spacing: 12

                VcCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 276
                    Layout.minimumHeight: 260
                    theme: root.theme

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        PanelHeader {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "mapping"
                            title: "Active Inputs"
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
                            spacing: 8

                            Repeater {
                                model: root.uiState.gamepadInputs

                                delegate: RowLayout {
                                    required property var modelData

                                    Layout.fillWidth: true

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.keyName
                                        color: root.theme.textSecondary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 11
                                    }

                                    Text {
                                        text: modelData.value
                                        color: modelData.value === "Pressed"
                                            ? root.theme.accent
                                            : root.theme.textPrimary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                VcCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 250
                    theme: root.theme

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        PanelHeader {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "settings"
                            title: "Backend"
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
                            spacing: 10

                            Text {
                                text: "Output device"
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                            }

                            VcComboBox {
                                Layout.fillWidth: true
                                theme: root.theme
                                model: [
                                    "Mock",
                                    "Xbox-compatible (planned)",
                                    "PlayStation (planned)"
                                ]
                                currentIndex: 0
                                enabled: false
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.theme.borderSubtle
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    Layout.fillWidth: true
                                    text: "Neutralize on tracking loss"
                                    color: root.theme.textPrimary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                }

                                VcSwitch {
                                    theme: root.theme
                                    checked: true
                                    enabled: false
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "All active axes and buttons should return to neutral "
                                    + "when the required hand is lost or the pipeline stops."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                wrapMode: Text.WordWrap
                            }

                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }
                }
            }
        }
    }
}
