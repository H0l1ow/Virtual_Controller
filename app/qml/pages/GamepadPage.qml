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
    property bool mockData: false

    // Derive layout constraints from the viewport, never from a sibling's
    // implicit width. The previous two-column GridLayout could collapse the
    // first VcCard to zero width and hide the entire controller diagram.
    readonly property real pageContentWidth: Math.max(1, gamepadScroll.availableWidth - 2 * theme.pageMargin)
    readonly property bool wideLayout: pageContentWidth >= 860
    readonly property real metricTileWidth: Math.max(1, (pageContentWidth - (pageContentWidth >= 760 ? 30 : 10)) / (pageContentWidth >= 760 ? 4 : 2))

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
            width: root.pageContentWidth
            x: root.theme.pageMargin
            y: root.theme.pageMargin
            spacing: 12

            PageToolbar {
                Layout.fillWidth: true
                theme: root.theme
                title: qsTr("Gamepad Monitor")
                subtitle: qsTr("Xbox and PlayStation layouts. Gamepad input is not connected yet.")

                StatusBadge {
                    theme: root.theme
                    text: root.mockData ? qsTr("Mock backend") : qsTr("Layout preview")
                    kind: "warning"
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
                    Layout.minimumWidth: 0
                    text: qsTr("Display layout only — no physical gamepad backend")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }

            // Diagram always occupies a full-width card ABOVE diagnostic info.
            // It is not conditional on mockData, backend availability, or a
            // GridLayout column with zero implicit width.
            VcCard {
                id: diagramCard
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: root.pageContentWidth
                Layout.preferredHeight: Math.max(390, Math.min(560, gamepadScroll.height * 0.70))
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "gamepad"
                        title: root.controllerLayout === "playstation"
                            ? qsTr("PlayStation controller") : qsTr("Xbox controller")
                        statusText: root.mockData ? qsTr("Demo inputs") : qsTr("Layout preview")
                        statusKind: "warning"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    Item {
                        id: diagramArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 285
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        Layout.topMargin: 7
                        Layout.bottomMargin: 5
                        clip: true

                        Rectangle {
                            anchors.fill: parent
                            color: root.theme.backgroundAlt
                            radius: root.theme.controlRadius
                        }

                        GamepadVisual {
                            id: gamepadVisual
                            anchors.centerIn: parent
                            theme: root.theme
                            controllerLayout: root.controllerLayout

                            readonly property real drawingWidth: Math.max(1, diagramArea.width - 28)
                            readonly property real drawingHeight: Math.max(1, diagramArea.height - 18)
                            width: Math.max(1, Math.min(drawingWidth, drawingHeight * 720 / 440))
                            height: width * 440 / 720
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: root.mockData
                            ? qsTr("Demonstration data — not physical gamepad input")
                            : qsTr("Controller layout visible even without a connected backend")
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }

            // Placeholder values do not pretend to be physical inputs. Keep
            // them below the actual gamepad instead of replacing its drawing.
            GridLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                columns: root.pageContentWidth >= 760 ? 4 : 2
                columnSpacing: 10
                rowSpacing: 10

                MetricTile {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.metricTileWidth
                    theme: root.theme
                    iconName: "target"
                    value: root.mockData ? root.uiState.leftStickValue : qsTr("N/A")
                    label: qsTr("Left Stick X / Y")
                }
                MetricTile {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.metricTileWidth
                    theme: root.theme
                    iconName: "target"
                    value: root.mockData ? root.uiState.rightStickValue : qsTr("N/A")
                    label: qsTr("Right Stick X / Y")
                }
                MetricTile {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.metricTileWidth
                    theme: root.theme
                    iconName: "play"
                    value: root.mockData ? root.uiState.triggerValue.split("/")[0].trim() : qsTr("N/A")
                    label: root.controllerLayout === "playstation" ? "L2" : "LT"
                }
                MetricTile {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.metricTileWidth
                    theme: root.theme
                    iconName: "play"
                    value: root.mockData
                        ? (root.uiState.triggerValue.split("/").length > 1
                           ? root.uiState.triggerValue.split("/")[1].trim() : "0.00")
                        : qsTr("N/A")
                    label: root.controllerLayout === "playstation" ? "R2" : "RT"
                }
            }

            // Compact diagnostics, explicitly sized in either one or two
            // columns. These must never displace or hide the diagram.
            GridLayout {
                id: detailsGrid
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                columns: root.wideLayout ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.wideLayout ? (root.pageContentWidth - 12) / 2 : root.pageContentWidth
                    Layout.preferredHeight: root.mockData ? 270 : 146
                    theme: root.theme

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        PanelHeader {
                            Layout.fillWidth: true
                            theme: root.theme
                            iconName: "mapping"
                            title: root.mockData ? qsTr("Demo inputs") : qsTr("Input states")
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

                            Text {
                                visible: !root.mockData
                                Layout.fillWidth: true
                                text: qsTr("Gamepad input monitoring is not available yet. Buttons in the diagram represent layout only.")
                                wrapMode: Text.WordWrap
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 12
                            }
                            Repeater {
                                model: root.mockData ? root.uiState.gamepadInputs : []
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
                                            ? root.theme.accent : root.theme.textPrimary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }

                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.wideLayout ? (root.pageContentWidth - 12) / 2 : root.pageContentWidth
                    Layout.preferredHeight: root.mockData ? 270 : 146
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
                            spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: root.mockData ? qsTr("Demonstration data — not physical gamepad input")
                                    : qsTr("No gamepad backend connected")
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Neutralize on tracking loss")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
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
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
}
