import QtQuick
import QtQml
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int selectedRow: 1
    readonly property var selectedMapping: uiState.mappingRows.length > selectedRow
        ? uiState.mappingRows[selectedRow]
        : ({})

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Gesture Mapping"
            subtitle: "Map hand gestures and continuous controls to logical actions."

            VcComboBox {
                id: mappingProfileCombo
                theme: root.theme
                model: root.uiState.profileNames
                currentIndex: Math.max(0, root.uiState.profileNames.indexOf(root.uiState.activeProfile))
                implicitWidth: 150
                onActivated: root.uiState.activeProfile = currentText
            }
            VcButton {
                theme: root.theme
                text: "Import"
                iconName: "download"
                implicitWidth: 100
                enabled: false
            }
            VcButton {
                theme: root.theme
                text: "Export"
                iconName: "upload"
                implicitWidth: 100
                enabled: false
            }
            VcButton {
                theme: root.theme
                text: "Add mapping"
                iconName: "plus"
                implicitWidth: 124
                enabled: false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.preferredWidth: 224
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "mapping"
                        title: "Sources"
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 12
                        spacing: 7

                        Text {
                            text: "HAND"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 1
                        }
                        Repeater {
                            model: ["All hands", "Left hand", "Right hand", "Two hands"]
                            delegate: Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                color: modelData === "All hands" ? root.theme.surfaceHover : "transparent"
                                border.width: modelData === "All hands" ? 1 : 0
                                border.color: root.theme.border
                                radius: root.theme.controlRadius

                                Text {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 10
                                    text: modelData
                                    color: root.theme.textSecondary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: 6
                            color: root.theme.borderSubtle
                        }
                        Text {
                            text: "SOURCE TYPE"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 1
                        }
                        Repeater {
                            model: ["Gestures", "Continuous", "Two-hand"]
                            delegate: Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                                color: "transparent"
                                Text {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 10
                                    text: modelData
                                    color: root.theme.textSecondary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
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
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        Layout.leftMargin: 14
                        Layout.rightMargin: 14
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: "Mappings"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        VcTextField {
                            theme: root.theme
                            placeholderText: "Search mappings (planned)"
                            implicitWidth: 230
                            enabled: false
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: root.theme.backgroundAlt
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            Repeater {
                                model: [
                                    {
                                        title: "Hand",
                                        width: 60
                                    },
                                    {
                                        title: "Source",
                                        width: 140
                                    },
                                    {
                                        title: "Type",
                                        width: 84
                                    },
                                    {
                                        title: "Action",
                                        width: 130
                                    },
                                    {
                                        title: "Output",
                                        width: 80
                                    },
                                    {
                                        title: "State",
                                        width: 72
                                    }
                                ]
                                delegate: Text {
                                    required property var modelData
                                    Layout.preferredWidth: modelData.width
                                    Layout.fillWidth: modelData.title === "Action"
                                    text: modelData.title.toUpperCase()
                                    color: root.theme.textMuted
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: .6
                                }
                            }
                        }
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentHeight: rowsColumn.implicitHeight
                        clip: true

                        Column {
                            id: rowsColumn
                            width: parent.width
                            Repeater {
                                model: root.uiState.mappingRows
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: rowsColumn.width
                                    height: 56
                                    color: root.selectedRow === index ? root.theme.surfaceHover : "transparent"
                                    border.width: 0

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        height: 1
                                        color: root.theme.borderSubtle
                                    }
                                    Rectangle {
                                        visible: root.selectedRow === index
                                        anchors.left: parent.left
                                        width: 2
                                        height: parent.height
                                        color: root.theme.accent
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 8
                                        Text {
                                            Layout.preferredWidth: 60
                                            text: modelData.hand
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        Text {
                                            Layout.preferredWidth: 140
                                            text: modelData.source
                                            color: root.theme.textPrimary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        TagChip {
                                            Layout.preferredWidth: 84
                                            theme: root.theme
                                            text: modelData.type
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 130
                                            text: modelData.action
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.preferredWidth: 80
                                            text: modelData.output
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        StatusBadge {
                                            Layout.preferredWidth: 72
                                            theme: root.theme
                                            text: modelData.state
                                            kind: "success"
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.selectedRow = index
                                    }
                                }
                            }
                        }
                    }
                }
            }

            VcCard {
                Layout.preferredWidth: 292
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "settings"
                        title: "Mapping Details"
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
                            text: "Hand"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        VcComboBox {
                            Layout.fillWidth: true
                            theme: root.theme
                            model: ["Left", "Right", "Either", "Two hands"]
                            currentIndex: Math.max(0, ["Left", "Right", "Either", "Two hands"].indexOf(root.selectedMapping.hand || "Right"))
                            enabled: false
                        }
                        Text {
                            text: "Source"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        VcComboBox {
                            Layout.fillWidth: true
                            theme: root.theme
                            model: ["Open Hand", "Pinch", "Point", "Fist", "Thumbs Up", "Thumbs Down"]
                            currentIndex: Math.max(0, ["Open Hand", "Pinch", "Point", "Fist", "Thumbs Up", "Thumbs Down"].indexOf(root.selectedMapping.source || "Pinch"))
                            enabled: false
                        }
                        Text {
                            text: "Action"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        VcComboBox {
                            Layout.fillWidth: true
                            theme: root.theme
                            model: ["Left click", "Right click", "Move cursor", "Scroll up", "Scroll down", "Pause / resume"]
                            currentIndex: Math.max(0, ["Left click", "Right click", "Move cursor", "Scroll up", "Scroll down", "Pause / resume"].indexOf(root.selectedMapping.action || "Left click"))
                            enabled: false
                        }
                        Text {
                            text: "Behavior"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        VcComboBox {
                            Layout.fillWidth: true
                            theme: root.theme
                            model: ["Press", "Hold", "Toggle", "Continuous"]
                            currentIndex: root.selectedMapping.type === "Continuous" ? 3 : 0
                            enabled: false
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: 3
                            color: root.theme.borderSubtle
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: "Enabled"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                            VcSwitch {
                                theme: root.theme
                                checked: root.selectedMapping.state === "Enabled"
                                enabled: false
                            }
                        }
                        Item {
                            Layout.fillHeight: true
                        }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: "Save mapping"
                            iconName: "save"
                            enabled: false
                        }
                    }
                }
            }
        }
    }
    Connections {
        target: root.uiState

        function onActiveProfileChanged() {
            const index = root.uiState.profileNames.indexOf(root.uiState.activeProfile)
            if (index >= 0 && mappingProfileCombo.currentIndex !== index)
                mappingProfileCombo.currentIndex = index
        }
    }

}
