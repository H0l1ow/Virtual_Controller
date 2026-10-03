import QtQuick
import QtQuick.Controls

TextField {
    id: root
    required property QtObject theme
    color: theme.textPrimary
    placeholderTextColor: theme.textMuted
    selectionColor: theme.accentDim
    selectedTextColor: theme.textPrimary
    font.family: theme.fontFamily
    font.pixelSize: 12
    leftPadding: 12
    rightPadding: 12
    implicitHeight: 40
    opacity: enabled ? 1.0 : 0.45
    background: Rectangle {
        color: root.theme.surfaceRaised
        border.width: 1
        border.color: root.activeFocus ? root.theme.borderStrong : root.theme.border
        radius: root.theme.controlRadius
    }
}
