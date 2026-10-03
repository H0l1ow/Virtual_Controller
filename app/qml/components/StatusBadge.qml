import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject theme
    property string text: "Active"
    property string kind: "success"
    property bool showDot: true

    readonly property color statusColor: kind === "success"
        ? theme.success
        : kind === "warning"
            ? theme.warning
            : kind === "error"
                ? theme.error
                : theme.neutral

    implicitWidth: row.implicitWidth + 14
    implicitHeight: 24
    radius: theme.smallRadius
    color: "transparent"

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 7

        Rectangle {
            visible: root.showDot
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            radius: 4
            color: root.statusColor
        }

        Text {
            text: root.text
            color: root.statusColor
            font.family: root.theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }
}
