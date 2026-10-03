import QtQuick

Rectangle {
    id: root
    required property QtObject theme
    property string iconName: "camera"
    property bool checked: false
    signal clicked()

    opacity: enabled ? 1.0 : 0.45

    implicitWidth: 42
    implicitHeight: 42
    radius: theme.controlRadius
    color: root.enabled && mouse.containsMouse
        ? theme.surfaceHover
        : Qt.rgba(theme.background.r, theme.background.g, theme.background.b, 0.82)
    border.width: 1
    border.color: checked ? theme.accentDim : theme.borderStrong

    VcIcon {
        anchors.centerIn: parent
        width: 22
        height: 22
        name: root.iconName
        color: root.checked ? root.theme.accent : root.theme.textPrimary
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
