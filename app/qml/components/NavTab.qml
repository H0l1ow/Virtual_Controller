import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme
    property string text: "Controller"
    property string iconName: "monitor"
    property bool selected: false
    signal clicked()

    implicitWidth: 132
    implicitHeight: 72

    Rectangle {
        anchors.fill: parent
        anchors.margins: 4
        radius: root.theme.controlRadius
        color: root.selected
            ? root.theme.surface
            : mouse.containsMouse
                ? root.theme.surfaceHover
                : "transparent"
        border.width: root.selected ? 1 : 0
        border.color: root.theme.borderSubtle
    }

    RowLayout {
        anchors.centerIn: parent
        spacing: 12

        VcIcon {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            name: root.iconName
            color: root.selected
                ? root.theme.textPrimary
                : root.theme.textSecondary
        }

        Text {
            text: root.text
            color: root.selected
                ? root.theme.textPrimary
                : root.theme.textSecondary
            font.family: root.theme.fontFamily
            font.pixelSize: 14
            font.weight: root.selected ? Font.DemiBold : Font.Normal
        }
    }

    Rectangle {
        visible: root.selected
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        height: 2
        radius: 1
        color: root.theme.textSecondary
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
