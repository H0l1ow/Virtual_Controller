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
            iconName: "settings"
            title: "Settings"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.border
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            spacing: 9

            RowLayout {
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: "Sensitivity"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Text {
                    text: Math.round(sensitivitySlider.value) + "%"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
            }

            VcSlider {
                id: sensitivitySlider
                Layout.fillWidth: true
                theme: root.theme
                value: root.uiState.sensitivity
                onMoved: root.uiState.sensitivity = Math.round(value)
            }

            Text {
                Layout.fillWidth: true
                text: "Adjust how responsive the cursor is to hand movement."
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root.theme.borderSubtle
            }

            Text {
                text: "Active Profile"
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            VcComboBox {
                id: profileCombo
                Layout.fillWidth: true
                theme: root.theme
                model: root.uiState.profileNames
                currentIndex: Math.max(0, root.uiState.profileNames.indexOf(root.uiState.activeProfile))
                onActivated: root.uiState.activeProfile = currentText
            }

            Text {
                Layout.fillWidth: true
                text: "Profiles keep gesture mappings and control tuning together."
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Connections {
        target: root.uiState

        function onSensitivityChanged() {
            if (!sensitivitySlider.pressed
                    && Math.round(sensitivitySlider.value) !== root.uiState.sensitivity) {
                sensitivitySlider.value = root.uiState.sensitivity
            }
        }

        function onActiveProfileChanged() {
            const index = root.uiState.profileNames.indexOf(root.uiState.activeProfile)
            if (index >= 0 && profileCombo.currentIndex !== index)
                profileCombo.currentIndex = index
        }
    }
}
