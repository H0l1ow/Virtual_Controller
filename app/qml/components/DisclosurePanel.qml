import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    required property QtObject theme
    property string title: qsTr("Advanced options")
    property string description: ""
    property string iconName: "settings"
    property bool expanded: false
    property int bodyHeight: 234
    property Component bodyComponent

    implicitHeight: expanded ? 54 + bodyHeight : 54
    Behavior on implicitHeight { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
    clip: true
    radius: theme.cardRadius
    color: theme.surface
    border.color: theme.border
    border.width: 1

    RowLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        height: 42
        spacing: 11

        VcIcon {
            Layout.preferredWidth: 21
            Layout.preferredHeight: 21
            name: root.iconName
            color: root.theme.accent
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1
            Text {
                text: root.title
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            Text {
                visible: root.description.length > 0
                text: root.description
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
        Text {
            text: root.expanded ? "⌃" : "⌄"
            color: root.theme.textSecondary
            font.pixelSize: 20
        }
    }
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 54
        cursorShape: Qt.PointingHandCursor
        onClicked: root.expanded = !root.expanded
    }
    Rectangle {
        visible: root.expanded
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 53
        height: 1
        color: root.theme.borderSubtle
    }
    Loader {
        id: details
        active: root.expanded
        visible: root.expanded
        sourceComponent: root.bodyComponent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        height: Math.max(0, root.bodyHeight - 68)
    }
}
