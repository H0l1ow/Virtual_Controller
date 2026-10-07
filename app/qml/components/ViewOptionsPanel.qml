import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject theme

    property bool showCameraStatus: true
    property bool showGestureHints: true
    property bool showMetrics: true
    property bool showHandBadges: true
    property bool showTrackingPanel: false
    property bool showControlsPanel: false
    property bool showSettingsPanel: false

    signal optionRequested(string key, bool visible)
    signal showAllRequested()

    width: 286
    height: content.implicitHeight + 24
    radius: root.theme.cardRadius
    color: Qt.rgba(
        root.theme.background.r,
        root.theme.background.g,
        root.theme.background.b,
        0.94
    )
    border.width: 1
    border.color: root.theme.borderStrong

    readonly property var options: [
        { key: "cameraStatus", label: "Camera status", enabled: root.showCameraStatus },
        { key: "gestureHints", label: "Gesture hints", enabled: root.showGestureHints },
        { key: "metrics", label: "Performance metrics", enabled: root.showMetrics },
        { key: "handBadges", label: "Hand state badges", enabled: root.showHandBadges },
        { key: "trackingPanel", label: "Hand Tracking panel", enabled: root.showTrackingPanel },
        { key: "controlsPanel", label: "Controls panel", enabled: root.showControlsPanel },
        { key: "settingsPanel", label: "Settings panel", enabled: root.showSettingsPanel }
    ]

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 5

            VcIcon {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                name: "menu"
                color: root.theme.accent
            }

            Text {
                Layout.fillWidth: true
                text: "Fullscreen UI"
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.bottomMargin: 3
            color: root.theme.borderSubtle
        }

        Repeater {
            model: root.options

            delegate: Rectangle {
                id: optionRow
                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 34
                radius: root.theme.smallRadius
                color: rowMouse.containsMouse
                    ? root.theme.surfaceHover
                    : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 8
                    text: optionRow.modelData.label
                    color: root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 8
                    width: 34
                    height: 18
                    radius: 9
                    color: optionRow.modelData.enabled
                        ? root.theme.accentDim
                        : root.theme.borderStrong
                    border.width: 1
                    border.color: optionRow.modelData.enabled
                        ? root.theme.accent
                        : root.theme.borderStrong

                    Rectangle {
                        x: optionRow.modelData.enabled ? parent.width - width - 3 : 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: 12
                        height: 12
                        radius: 6
                        color: optionRow.modelData.enabled
                            ? root.theme.accent
                            : root.theme.textSecondary

                        Behavior on x {
                            NumberAnimation { duration: 90 }
                        }
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.optionRequested(
                        optionRow.modelData.key,
                        !optionRow.modelData.enabled
                    )
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 3
            Layout.bottomMargin: 5
            color: root.theme.borderSubtle
        }

        VcButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            theme: root.theme
            iconName: "reset"
            text: "Show all"
            variant: "secondary"
            onClicked: root.showAllRequested()
        }
    }
}
