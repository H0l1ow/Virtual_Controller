import QtQuick
import QtQuick.Layouts

VcCard {
    id: root

    required property QtObject uiState

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            theme: root.theme
            iconName: "target"
            title: root.uiState.cursorControlAvailable
                ? "Cursor Tracking"
                : "Hand Tracking"
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
                    color: "#111920"
                    border.width: 1
                    border.color: root.theme.border
                    radius: root.theme.smallRadius
                    clip: true

                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.leftMargin: 8
                        anchors.topMargin: 6
                        text: "Desktop"
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
                                screen.width * root.uiState.cursorX / 1920
                            )
                        )
                        y: Math.max(
                            18,
                            Math.min(
                                screen.height - height - 8,
                                screen.height * root.uiState.cursorY / 1080
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
                    text: "Cursor Position"
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: !root.uiState.cursorControlAvailable
                        ? "Not enabled in M1"
                        : (root.uiState.pipelineRunning && root.uiState.rightTracked
                            ? "X: " + root.uiState.cursorX + "    Y: " + root.uiState.cursorY
                            : "X: --    Y: --")
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: root.theme.borderSubtle
                }

                Text {
                    text: "Smoothing"
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: root.uiState.smoothing ? "On" : "Off"
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
                    text: "Tracking Status"
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }

                Text {
                    text: root.uiState.trackingStatus
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

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 14
            Layout.bottomMargin: 14
            spacing: 8

            MetricTile {
                Layout.fillWidth: true
                theme: root.theme
                iconName: "target"
                value: root.uiState.fps.toString()
                label: "FPS"
            }

            MetricTile {
                Layout.fillWidth: true
                theme: root.theme
                iconName: "monitor"
                value: root.uiState.resolution
                label: "Resolution"
            }

            MetricTile {
                Layout.fillWidth: true
                theme: root.theme
                iconName: "play"
                value: root.uiState.latencyMs + " ms"
                label: "Latency"
            }
        }
    }
}
