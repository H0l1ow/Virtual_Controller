import VirtualController.I18n 1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState
    property string controllerLayout: "xbox"

    function playstationInputLabel(name) {
        switch (name) {
        case "A": return "Cross (×)"
        case "B": return "Circle (○)"
        case "X": return "Square (□)"
        case "Y": return "Triangle (△)"
        case "LB / RB": return "L1 / R1"
        default: return name
        }
    }

    function localizedStatus(value) {
        LanguageManager.language
        return qsTr(value)
    }

    ScrollView {
        id: gamepadScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        width: Math.max(1, gamepadScroll.availableWidth - 2 * root.theme.pageMargin)
        x: root.theme.pageMargin
        y: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Gamepad Monitor")
            subtitle: qsTr("Live logical controller state. The virtual device backend will be connected in a later stage.")

            StatusBadge {
                theme: root.theme
                text: qsTr("Mock backend")
                kind: "warning"
            }

            StatusBadge {
                theme: root.theme
                text: root.uiState.outputArmed ? qsTr("Output armed") : qsTr("Output disarmed")
                kind: root.uiState.outputArmed ? "success" : "neutral"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            VcButton {
                theme: root.theme
                text: "Xbox"
                variant: root.controllerLayout === "xbox" ? "primary" : "secondary"
                onClicked: root.controllerLayout = "xbox"
            }
            VcButton {
                theme: root.theme
                text: "PlayStation"
                variant: root.controllerLayout === "playstation" ? "primary" : "secondary"
                onClicked: root.controllerLayout = "playstation"
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Display layout only — no physical gamepad backend")
                color: root.theme.textMuted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1120 ? 2 : 1
            columnSpacing: 12
            rowSpacing: 12

            VcCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 570
                Layout.minimumWidth: 0
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        showIcon: false
                        title: qsTr("Controller State")
                        statusText: root.uiState.pipelineRunning ? qsTr("Live") : qsTr("Stopped")
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
                            controllerLayout: root.controllerLayout

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

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        columns: width >= 700 ? 4 : 2
                        columnSpacing: 8
                        rowSpacing: 8
                        Layout.preferredHeight: columns === 4 ? 58 : 124

                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "target"
                            value: root.uiState.leftStickValue
                            label: qsTr("Left Stick X / Y")
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "target"
                            value: root.uiState.rightStickValue
                            label: qsTr("Right Stick X / Y")
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "play"
                            value: root.uiState.triggerValue.split("/")[0].trim()
                            label: root.controllerLayout === "playstation" ? "L2" : "LT"
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "play"
                            value: root.uiState.triggerValue.split("/").length > 1
                                   ? root.uiState.triggerValue.split("/")[1].trim() : "0.00"
                            label: root.controllerLayout === "playstation" ? "R2" : "RT"
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: width >= 1120 ? 320 : 720
                Layout.minimumWidth: 0
                Layout.preferredHeight: 570
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
                            title: qsTr("Active Inputs")
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
                                        text: root.controllerLayout === "playstation"
                                            ? root.playstationInputLabel(modelData.keyName)
                                            : modelData.keyName
                                        color: root.theme.textSecondary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 11
                                    }

                                    Text {
                                        text: root.localizedStatus(modelData.value)
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
                            title: qsTr("Backend")
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
                                text: qsTr("Output device")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Mock backend — preview only")
                                color: root.theme.textSecondary
                                font.pixelSize: 12
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
                                    text: qsTr("Neutralize on tracking loss")
                                    color: root.theme.textPrimary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                }


                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("All active axes and buttons should return to neutral ")
                                    + qsTr("when the required hand is lost or the pipeline stops.")
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
}
