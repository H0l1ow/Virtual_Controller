import QtQuick

Rectangle {
    id: root

    required property QtObject theme
    property string text: "Enabled"
    property string kind: "neutral"

    readonly property color c: kind === "success"
        ? theme.accent
        : kind === "warning"
            ? theme.warning
            : kind === "error"
                ? theme.error
                : theme.textSecondary

    implicitWidth: label.implicitWidth + 16
    implicitHeight: 24
    color: Qt.rgba(c.r, c.g, c.b, 0.08)
    border.width: 1
    border.color: Qt.rgba(c.r, c.g, c.b, 0.26)
    radius: theme.smallRadius

    Text {
        id: label

        anchors.centerIn: parent
        text: root.text
        color: root.c
        font.family: root.theme.fontFamily
        font.pixelSize: 10
        font.weight: Font.DemiBold
    }
}
