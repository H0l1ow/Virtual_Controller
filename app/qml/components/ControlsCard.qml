import QtQuick
import QtQml
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
            iconName: "play"
            title: qsTr("Controls")
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
                iconName: root.uiState.canStop ? "reset" : "play"
                text: root.uiState.runtimeStateName === "Stopping"
                    ? qsTr("Stopping...")
                    : (root.uiState.canStop ? qsTr("Stop tracking") : qsTr("Start tracking"))
                variant: "secondary"
                enabled: root.uiState.canStart || root.uiState.canStop
                onClicked: root.uiState.togglePipeline()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: qsTr("System output")
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                Text {
                    text: !root.uiState.outputAvailable
                        ? qsTr("Unavailable")
                        : (root.uiState.outputArmed ? qsTr("Armed") : qsTr("Disarmed"))
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
                        && root.uiState.runtimeStateName === "Running"
                    checked: root.uiState.outputArmed
                    onToggled: root.uiState.setOutputArmed(checked)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                VcButton {
                    Layout.fillWidth: true
                    theme: root.theme
                    iconName: "target"
                    text: qsTr("Calibrate")
                    enabled: false
                }

                VcButton {
                    Layout.fillWidth: true
                    theme: root.theme
                    iconName: "reset"
                    text: qsTr("Reset")
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
                    text: root.uiState.outputError.length > 0
                        ? root.uiState.outputError
                        : (root.uiState.outputAvailable
                            ? qsTr("M4: cursor movement and mapped mouse/keyboard actions share the protected output path. ")
                                + qsTr("System output is opt-in; STOP, stale frames and F8 release everything.")
                            : qsTr("Windows mouse/keyboard output is not available in this build."))
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
