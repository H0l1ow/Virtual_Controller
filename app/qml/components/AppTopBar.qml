import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    required property QtObject theme
    required property var windowRef
    property int currentIndex: 0
    signal pageSelected(int index)

    color: theme.topBar
    border.width: 1
    border.color: theme.border

    readonly property var navItems: [
        {
            title: "Controller",
            icon: "monitor"
        },
        {
            title: "Mapping",
            icon: "mapping"
        },
        {
            title: "Gestures",
            icon: "hand"
        },
        {
            title: "Gamepad",
            icon: "gamepad"
        },
        {
            title: "Settings",
            icon: "settings"
        }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.preferredWidth: 320
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 12
                spacing: 12

                HandLogo {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    color: root.theme.textPrimary
                }

                Row {
                    spacing: 0
                    Text {
                        text: "Virtual"
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 25
                        font.weight: Font.Bold
                    }
                    Text {
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
                onPressed: root.windowRef.startSystemMove()
                onDoubleClicked: {
                    if (root.windowRef.visibility === Window.Maximized)
                        root.windowRef.showNormal()
                    else
                        root.windowRef.showMaximized()
                }
            }
        }

        Repeater {
            model: root.navItems
            delegate: NavTab {
                required property var modelData
                required property int index
                Layout.fillHeight: true
                Layout.preferredWidth: 132
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
                onPressed: root.windowRef.startSystemMove()
                onDoubleClicked: {
                    if (root.windowRef.visibility === Window.Maximized)
                        root.windowRef.showNormal()
                    else
                        root.windowRef.showMaximized()
                }
            }
        }

        WindowControlButton {
            Layout.fillHeight: true
            theme: root.theme
            symbol: "—"
            onClicked: root.windowRef.showMinimized()
        }
        WindowControlButton {
            Layout.fillHeight: true
            theme: root.theme
            symbol: root.windowRef.visibility === Window.Maximized ? "❐" : "□"
            onClicked: {
                if (root.windowRef.visibility === Window.Maximized)
                    root.windowRef.showNormal()
                else
                    root.windowRef.showMaximized()
            }
        }
        WindowControlButton {
            Layout.fillHeight: true
            theme: root.theme
            symbol: "×"
            danger: true
            onClicked: root.windowRef.close()
        }
    }
}
