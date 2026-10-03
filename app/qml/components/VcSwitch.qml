import QtQuick
import QtQuick.Controls

Switch {
    id: root
    required property QtObject theme
    implicitWidth: 42
    implicitHeight: 22
    opacity: enabled ? 1.0 : 0.45

    indicator: Rectangle {
        implicitWidth: 40
        implicitHeight: 20
        x: root.leftPadding
        y: parent.height / 2 - height / 2
        radius: 10
        color: root.checked ? root.theme.accentDim : root.theme.borderStrong
        border.width: 1
        border.color: root.checked ? root.theme.accent : root.theme.borderStrong
        Rectangle {
            x: root.checked ? parent.width - width - 3 : 3
            y: 3
            width: 12
            height: 12
            radius: 6
            color: root.checked ? root.theme.accent : root.theme.textSecondary
            Behavior on x {
                NumberAnimation {
                    duration: 100
                }
            }
        }
    }
    contentItem: Item {
    }
}
