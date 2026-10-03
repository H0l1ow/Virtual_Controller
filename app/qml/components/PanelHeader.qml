import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme
    property string iconName: "monitor"
    property string title: "Panel"
    property string statusText: ""
    property string statusKind: "success"
    property bool showIcon: true

    implicitHeight: 50
    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 11

        VcIcon {
            visible: root.showIcon
            Layout.preferredWidth: root.showIcon ? 22 : 0
            Layout.preferredHeight: root.showIcon ? 22 : 0
            name: root.iconName
            color: root.theme.textSecondary
        }

        Text {
            Layout.fillWidth: true
            text: root.title
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 16
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        StatusBadge {
            visible: root.statusText.length > 0
            theme: root.theme
            text: root.statusText
            kind: root.statusKind
        }
    }
}
