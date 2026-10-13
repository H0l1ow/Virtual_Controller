import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    required property QtObject theme
    required property var windowRef
    property int currentIndex: 0
    signal pageSelected(int index)

    function toggleWindowSize() {
        if (root.windowRef.controllerFullscreen) {
            root.windowRef.setControllerFullscreen(false)
            return
        }

        if (root.windowRef.visibility === Window.Maximized)
            root.windowRef.showNormal()
        else
            root.windowRef.showMaximized()
    }

    color: theme.topBar
    border.width: 1
    border.color: theme.border

    readonly property var navItems: [
        {
            title: qsTr("Controller"),
            icon: "monitor"
        },
        {
            title: qsTr("Mapping"),
            icon: "mapping"
        },
        {
            title: qsTr("Gestures"),
            icon: "hand"
        },
        {
            title: qsTr("Test"),
            icon: "target"
        },
        {
            title: qsTr("Gamepad"),
            icon: "gamepad"
        },
        {
            title: qsTr("ML"),
            icon: "ml"
        },
        {
            title: qsTr("Settings"),
            icon: "settings"
        }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.preferredWidth: root.width < 1150 ? 140 : root.width < 1450 ? 240 : 290
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 12
                spacing: 12

                HandLogo {
                    Layout.preferredWidth: root.width < 1100 ? 30 : 38
                    Layout.preferredHeight: root.width < 1100 ? 30 : 38
                    color: root.theme.textPrimary
                }

                Row {
                    spacing: 0
                    Text {
                        text: root.width < 1100 ? "VC" : "Virtual"
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 25
                        font.weight: Font.Bold
                    }
                    Text {
                        visible: root.width >= 1100
                        text: "Controller"
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 25
                        font.weight: Font.DemiBold
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: {
                    if (!root.windowRef.controllerFullscreen)
                        root.windowRef.startSystemMove()
                }
                onDoubleClicked: root.toggleWindowSize()
            }
        }

        Repeater {
            model: root.navItems
            delegate: NavTab {
                required property var modelData
                required property int index
                Layout.fillHeight: true
                Layout.preferredWidth: root.width < 1180 ? 60 : root.width < 1500 ? 100 : 116
                theme: root.theme
                text: modelData.title
                iconName: modelData.icon
                selected: index === root.currentIndex
                onClicked: root.pageSelected(index)
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: {
                    if (!root.windowRef.controllerFullscreen)
                        root.windowRef.startSystemMove()
                }
                onDoubleClicked: root.toggleWindowSize()
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 8
            Text {
                visible: root.width >= 1400
                text: qsTr("Profile")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 11
            }
            VcComboBox {
                id: quickProfile
                theme: root.theme
                implicitWidth: root.width < 1180 ? 110 : 148
                implicitHeight: 32
                model: root.windowRef.uiState.profileNames
                currentIndex: Math.max(0, root.windowRef.uiState.profileNames.indexOf(
                    root.windowRef.uiState.activeProfile))
                onActivated: function(index) {
                    if (index >= 0)
                        root.windowRef.uiState.setActiveProfile(currentText)
                }
            }
        }

        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: root.width < 1100 ? 40 : 48
            theme: root.theme
            symbol: "—"
            onClicked: root.windowRef.showMinimized()
        }
        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: root.width < 1100 ? 40 : 48
            theme: root.theme
            symbol: (root.windowRef.controllerFullscreen
                || root.windowRef.visibility === Window.Maximized) ? "❐" : "□"
            onClicked: root.toggleWindowSize()
        }
        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: root.width < 1100 ? 40 : 48
            theme: root.theme
            symbol: "×"
            danger: true
            onClicked: root.windowRef.close()
        }
    }
}
