import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    required property QtObject theme
    property string variant: "secondary" // primary | secondary | danger | ghost
    property string iconName: ""

    implicitHeight: 42
    // Respect translated text width instead of relying on a RowLayout's
    // computed width (which may be constrained by its parent).
    implicitWidth: Math.max(104, labelText.implicitWidth + 28
                            + (root.iconName.length > 0 ? 20 + contentRow.spacing : 0))
    hoverEnabled: enabled
    opacity: enabled ? 1.0 : 0.45

    readonly property color baseColor: {
        if (variant === "primary") return theme.accent
        if (variant === "danger") return theme.isLight ? "#FBE7EA" : "#271419"
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
            : root.variant === "primary" ? theme.accent : theme.borderStrong
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 9

        VcIcon {
            visible: root.iconName.length > 0
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            name: root.iconName
            color: root.variant === "danger" ? root.theme.error
                : root.variant === "primary" ? (root.theme.isLight ? "#FFFFFF" : "#031320") : root.theme.textPrimary
        }

        Text {
            id: labelText
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            elide: Text.ElideRight
            clip: true
            text: root.text
            color: root.variant === "danger" ? root.theme.error
                : root.variant === "primary" ? (root.theme.isLight ? "#FFFFFF" : "#031320") : root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
