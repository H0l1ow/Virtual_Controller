import QtQuick
import QtQuick.Controls

Slider {
    id: root

    required property QtObject theme
    from: 0
    to: 100
    value: 70
    implicitHeight: 24
    opacity: enabled ? 1.0 : 0.45

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.availableWidth
        height: 6
        color: root.theme.borderStrong
        radius: 3

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.theme.textSecondary
            radius: 3
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        color: root.pressed ? root.theme.textSecondary : root.theme.textPrimary
        border.width: 1
        border.color: root.theme.borderStrong
    }
}
