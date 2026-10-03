import QtQuick

Rectangle {
    required property QtObject theme

    color: theme.surface
    radius: theme.cardRadius
    border.width: 1
    border.color: theme.border
    clip: true
}
