import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        CameraStage {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 390
            theme: root.theme
            uiState: root.uiState
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 270
            Layout.minimumHeight: 235
            spacing: 12

            CursorTrackingCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1.18
                theme: root.theme
                uiState: root.uiState
            }

            ControlsCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 0.92
                theme: root.theme
                uiState: root.uiState
            }

            QuickSettingsCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 0.98
                theme: root.theme
                uiState: root.uiState
            }
        }
    }
}
