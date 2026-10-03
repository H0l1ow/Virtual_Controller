import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox {
    id: root
    required property QtObject theme
    implicitHeight: 40
    leftPadding: 12
    rightPadding: 34
    font.family: theme.fontFamily
    font.pixelSize: 12
    opacity: enabled ? 1.0 : 0.45

    delegate: ItemDelegate {
        width: root.width
        contentItem: Text {
            text: modelData
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: highlighted ? root.theme.surfaceHover : root.theme.surfaceRaised
            radius: root.theme.smallRadius
        }
    }

    indicator: Text {
        x: root.width - width - 12
        y: (root.height-height)/2 - 1
        text: "⌄"
        color: root.theme.textSecondary
        font.pixelSize: 17
    }

    contentItem: Text {
        leftPadding: 0
        rightPadding: root.indicator.width + root.spacing
        text: root.displayText
        color: root.theme.textPrimary
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: root.theme.surfaceRaised
        border.width: 1
        border.color: root.hovered ? root.theme.borderStrong : root.theme.border
        radius: root.theme.controlRadius
    }

    popup: Popup {
        y: root.height
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {
            }
        }
        background: Rectangle {
            color: root.theme.surfaceRaised
            border.width: 1
            border.color: root.theme.borderStrong
            radius: root.theme.controlRadius
        }
    }
}
