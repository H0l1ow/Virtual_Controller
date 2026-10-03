import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    required property QtObject theme
    property string variant: "secondary" // primary | secondary | danger | ghost
    property string iconName: ""

    implicitHeight: 42
    implicitWidth: Math.max(104, contentRow.implicitWidth + 28)
    hoverEnabled: enabled
    opacity: enabled ? 1.0 : 0.45

    readonly property color baseColor: {
        if (variant === "primary") return theme.surfaceRaised
        if (variant === "danger") return "#271419"
        if (variant === "ghost") return "transparent"
        return theme.surfaceRaised
    }

    background: Rectangle {
        radius: theme.controlRadius
        color: root.enabled && root.down
            ? theme.surfacePressed
            : root.enabled && root.hovered
                ? theme.surfaceHover
                : root.baseColor
        border.width: 1
        border.color: root.variant === "danger"
            ? Qt.rgba(theme.error.r, theme.error.g, theme.error.b, 0.55)
            : theme.borderStrong
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 9

        VcIcon {
            visible: root.iconName.length > 0
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            name: root.iconName
            color: root.variant === "danger" ? root.theme.error : root.theme.textPrimary
        }

        Text {
            Layout.fillWidth: true
            text: root.text
            color: root.variant === "danger" ? root.theme.error : root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
