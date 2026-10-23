import QtQuick
import QtQuick.Layouts

VcCard {
    id: root
    required property QtObject uiState
    translucent: true
    translucentOpacity: theme.isLight ? 0.78 : 0.68

    readonly property bool compact: width < 198 || height < 320

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 10 : 15
        spacing: root.compact ? 7 : 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 7
            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: root.uiState.outputActive ? root.theme.success
                    : root.uiState.outputArmed ? root.theme.warning : root.theme.textMuted
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("System output")
                font.family: root.theme.fontFamily
                font.pixelSize: root.compact ? 12 : 14
                font.weight: Font.DemiBold
                color: root.theme.textPrimary
                elide: Text.ElideRight
            }
            VcSwitch {
                theme: root.theme
                checked: root.uiState.outputArmed
                enabled: root.uiState.outputArmed
                    || (root.uiState.outputAvailable && root.uiState.runtimeStateName === "Running")
                onToggled: root.uiState.setOutputArmed(checked)
            }
        }
        Text {
            Layout.fillWidth: true
            text: root.uiState.outputArmed
                ? root.uiState.outputActive ? qsTr("Active") : qsTr("Auto-paused")
                : qsTr("Disarmed")
            color: root.uiState.outputActive ? root.theme.success
                : root.uiState.outputArmed ? root.theme.warning : root.theme.textSecondary
            font.family: root.theme.fontFamily
            font.pixelSize: root.compact ? 13 : 15
            elide: Text.ElideRight
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Active profile")
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: root.compact ? 13 : 15
        }
        Text {
            Layout.fillWidth: true
            text: root.uiState.activeProfile
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: root.compact ? 13 : 16
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Rectangle {
                Layout.preferredWidth: 7
                Layout.preferredHeight: 7
                radius: 4
                color: root.uiState.leftTracked ? root.theme.leftHand : root.theme.textMuted
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Left hand")
                color: root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: root.compact ? 13 : 15
            }
        }
        Text {
            Layout.fillWidth: true
            text: root.uiState.leftTracked ? qsTr(root.uiState.leftGesture) : qsTr("Not detected")
            color: root.uiState.leftTracked ? root.theme.leftHand : root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: root.compact ? 12 : 14
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Rectangle {
                Layout.preferredWidth: 7
                Layout.preferredHeight: 7
                radius: 4
                color: root.uiState.rightTracked ? root.theme.rightHand : root.theme.textMuted
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Right hand")
                color: root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: root.compact ? 13 : 15
            }
        }
        Text {
            Layout.fillWidth: true
            text: root.uiState.rightTracked ? qsTr(root.uiState.rightGesture) : qsTr("Not detected")
            color: root.uiState.rightTracked ? root.theme.rightHand : root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: root.compact ? 12 : 14
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true }
    }
}
