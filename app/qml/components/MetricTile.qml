import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject theme
    property string iconName: "monitor"
    property string value: "30"
    property string label: "FPS"

    implicitHeight: 56
    color: theme.backgroundAlt
    border.width: 1
    border.color: theme.border
    radius: theme.controlRadius
    clip: true

    RowLayout {
        anchors.centerIn: parent
        spacing: 10

        VcIcon {
            Layout.preferredWidth: 23
            Layout.preferredHeight: 23
            name: root.iconName
            color: root.theme.textSecondary
        }

        ColumnLayout {
            spacing: 0

            Text {
                text: root.value
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            Text {
                text: root.label
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 9
            }
        }
    }
}
