import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject theme
    property string title: "Page"
    property string subtitle: ""
    default property alias extraContent: extra.data

    implicitHeight: 62
    color: theme.backgroundAlt
    border.width: 1
    border.color: theme.border
    radius: theme.cardRadius
    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 14
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                text: root.title
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
            }
        }

        RowLayout {
            id: extra
            spacing: 8
        }
    }
}
