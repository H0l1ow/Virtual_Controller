import QtQuick
import QtQuick.Layouts

VcCard {
    id: root

    required property QtObject uiState
    property bool overlayMode: false

    translucent: root.overlayMode
    translucentOpacity: 0.80

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            theme: root.theme
            iconName: "target"
            title: root.uiState.cursorControlAvailable
                ? qsTr("Cursor Tracking")
                : qsTr("Hand Tracking")
            statusText: root.uiState.trackingStatus
            statusKind: root.uiState.trackingStatus === "Stable"
                ? "success"
                : root.uiState.pipelineRunning ? "warning" : "neutral"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.border
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 14
            spacing: 14

            Rectangle {
                id: desktopPreview

                Layout.preferredWidth: 188
                Layout.fillHeight: true
                Layout.minimumHeight: 114
                color: root.theme.backgroundAlt
                border.width: 1
                border.color: root.theme.borderStrong
                radius: root.theme.controlRadius
                clip: true

                Rectangle {
                    id: screen

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: stand.top
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 8
                    anchors.bottomMargin: 7
                    color: root.theme.surfaceRaised
                    border.width: 1
                    border.color: root.theme.border
                    radius: root.theme.smallRadius
                    clip: true

                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.leftMargin: 8
                        anchors.topMargin: 6
                        text: qsTr("Desktop")
                        color: root.theme.textFaint
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                    }

                    Text {
                        visible: root.uiState.cursorControlAvailable
                            && root.uiState.pipelineRunning
                            && root.uiState.rightTracked
                        x: Math.max(
                            8,
                            Math.min(
                                screen.width - width - 8,
                                screen.width * root.uiState.cursorX / Math.max(root.uiState.cursorScreenWidth, 1)
                            )
                        )
                        y: Math.max(
                            18,
                            Math.min(
                                screen.height - height - 8,
                                screen.height * root.uiState.cursorY / Math.max(root.uiState.cursorScreenHeight, 1)
                            )
                        )
                        text: "➤"
                        rotation: -45
                        color: root.theme.textPrimary
                        font.pixelSize: 22
                    }
                }

                Rectangle {
                    id: stand

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    width: 62
                    height: 6
                    radius: 3
                    color: root.theme.borderStrong
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 7

                Text {
                    text: qsTr("Cursor Position · Right hand")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: !root.uiState.cursorControlAvailable
                        ? qsTr("Not available")
                        : (root.uiState.pipelineRunning && root.uiState.rightTracked
                            ? "X: " + root.uiState.cursorX + "    Y: " + root.uiState.cursorY
                            : "X: --    Y: --")
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                StatusBadge {
                    visible: root.uiState.cursorFrozen
                    theme: root.theme
                    text: qsTr("CURSOR FROZEN")
                    kind: "warning"
                }

                StatusBadge {
                    visible: !root.uiState.cursorFrozen && root.uiState.cursorMovementLocked
                    theme: root.theme
                    text: qsTr("WAITING FOR MOVE GESTURE")
                    kind: "neutral"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: root.theme.borderSubtle
                }

                Text {
                    text: qsTr("Smoothing")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: root.uiState.smoothing ? qsTr("On") : qsTr("Off")
                    color: root.uiState.smoothing
                        ? root.theme.accent
                        : root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: root.theme.borderSubtle
                }

                Text {
                    text: qsTr("Tracking Status")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: qsTr(root.uiState.trackingStatus)
                    color: root.uiState.trackingStatus === "Stable"
                        ? root.theme.accent
                        : root.uiState.pipelineRunning
                            ? root.theme.warning
                            : root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

    }
}
