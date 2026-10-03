import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject theme
    property string gesture: "open"
    property string title: "Open Palm"
    property string action: "Move Cursor"
    property bool active: true

    implicitHeight: 42
    color: Qt.rgba(
        theme.background.r,
        theme.background.g,
        theme.background.b,
        0.68
    )
    border.width: 1
    border.color: Qt.rgba(
        theme.borderStrong.r,
        theme.borderStrong.g,
        theme.borderStrong.b,
        0.42
    )
    radius: theme.controlRadius

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 9
        spacing: 8

        GestureGlyph {
            Layout.preferredWidth: 27
            Layout.preferredHeight: 27
            gesture: root.gesture
            color: root.theme.textPrimary
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.action
                color: root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: 9
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            radius: 3.5
            color: root.active ? root.theme.accent : root.theme.textFaint
        }
    }
}
