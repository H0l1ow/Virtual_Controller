import QtQuick
import QtQml
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
            iconName: "play"
            title: "Controls"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.border
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            spacing: 8

            VcButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                theme: root.theme
                iconName: root.uiState.pipelineRunning ? "reset" : "play"
                text: root.uiState.pipelineRunning ? "Stop tracking" : "Start tracking"
                variant: "secondary"
                onClicked: root.uiState.togglePipeline()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: "System output"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                Text {
                    text: !root.uiState.outputAvailable
                        ? "M1: disabled"
                        : (root.uiState.outputArmed ? "Armed" : "Disarmed")
                    color: root.uiState.outputArmed
                        ? root.theme.accent
                        : root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }

                VcSwitch {
                    id: outputSwitch
                    theme: root.theme
                    enabled: root.uiState.outputAvailable
                        && root.uiState.pipelineRunning
                    checked: root.uiState.outputArmed
                    onToggled: root.uiState.outputArmed = checked
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                VcButton {
                    Layout.fillWidth: true
                    theme: root.theme
                    iconName: "target"
                    text: "Calibrate"
                    enabled: false
                }

                VcButton {
                    Layout.fillWidth: true
                    theme: root.theme
                    iconName: "reset"
                    text: "Reset"
                    enabled: !root.uiState.pipelineRunning
                    onClicked: root.uiState.resetSettingsDefaults()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root.theme.borderSubtle
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 11

                VcIcon {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    name: "info"
                    color: root.theme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: root.uiState.outputAvailable
                        ? "Tracking may stay live while system output is disarmed. "
                            + "Stopping the pipeline always neutralizes output."
                        : "M1 implements camera preview and two-hand tracking only. "
                            + "Desktop output remains disabled until the next milestone."
                    color: root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
    Connections {
        target: root.uiState

        function onOutputArmedChanged() {
            if (outputSwitch.checked !== root.uiState.outputArmed)
                outputSwitch.checked = root.uiState.outputArmed
        }
    }

}
