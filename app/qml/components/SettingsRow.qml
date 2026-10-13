import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property QtObject theme
    property string title: qsTr("Setting")
    property string description: ""
    default property alias controlContent: controlHost.data
    readonly property bool compact: width < 670
    implicitHeight: compact
        ? Math.max(78, labels.implicitHeight + controlHost.implicitHeight + 18)
        : Math.max(60, labels.implicitHeight + 18)

    GridLayout {
        anchors.fill: parent
        columns: root.compact ? 1 : 2
        columnSpacing: 16
        rowSpacing: 6

        ColumnLayout {
            id: labels
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: 3
            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
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
            Layout.fillWidth: root.compact
            Layout.minimumWidth: 0
            Layout.preferredWidth: root.compact ? root.width : Math.min(340, root.width * 0.46)
            Layout.alignment: root.compact ? Qt.AlignLeft : Qt.AlignVCenter
            spacing: 8
        }
    }
}
