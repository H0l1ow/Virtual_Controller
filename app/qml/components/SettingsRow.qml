import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property QtObject theme
    property string title: "Setting"
    property string description: ""
    default property alias controlContent: controlHost.data
    implicitHeight: Math.max(60, labels.implicitHeight + 18)

    RowLayout {
        anchors.fill: parent
        spacing: 20
        ColumnLayout {
            id: labels
            Layout.fillWidth: true
            spacing: 3
            Text {
                text: root.title
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: root.description
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            id: controlHost
            Layout.preferredWidth: 300
            Layout.alignment: Qt.AlignVCenter
            spacing: 8
        }
    }
}
