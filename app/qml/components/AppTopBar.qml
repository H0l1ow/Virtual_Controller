import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root
    required property QtObject theme
    required property var windowRef

    function toggleWindowSize() {
        if (root.windowRef.controllerFullscreen) {
            root.windowRef.setControllerFullscreen(false)
            return
        }
        if (root.windowRef.visibility === Window.Maximized)
            root.windowRef.showNormal()
        else
            root.windowRef.showMaximized()
    }

    color: theme.topBar
    border.width: 1
    border.color: theme.borderSubtle

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        spacing: 10

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: { if (!root.windowRef.controllerFullscreen) root.windowRef.startSystemMove() }
                onDoubleClicked: root.toggleWindowSize()
            }
        }

        StatusBadge {
            visible: root.width >= 730
            theme: root.theme
            text: root.windowRef.uiState.pipelineRunning ? qsTr("Tracking active") : qsTr("Tracking stopped")
            kind: root.windowRef.uiState.pipelineRunning ? "success" : "neutral"
        }
        VcComboBox {
            theme: root.theme
            implicitWidth: root.width < 980 ? 118 : 165
            implicitHeight: 33
            model: root.windowRef.uiState.profileNames
            currentIndex: Math.max(0, root.windowRef.uiState.profileNames.indexOf(root.windowRef.uiState.activeProfile))
            onActivated: function(index) {
                if (index >= 0)
                    root.windowRef.uiState.setActiveProfile(currentText)
            }
        }
        VcButton {
            theme: root.theme
            implicitHeight: 34
            iconName: root.windowRef.uiState.canStop ? "stop" : "play"
            text: root.windowRef.uiState.canStop ? qsTr("Stop") : qsTr("Start")
            variant: root.windowRef.uiState.canStop ? "secondary" : "primary"
            enabled: root.windowRef.uiState.canStart || root.windowRef.uiState.canStop
            onClicked: root.windowRef.uiState.togglePipeline()
        }

        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: 38
            theme: root.theme
            symbol: "—"
            onClicked: root.windowRef.showMinimized()
        }
        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: 38
            theme: root.theme
            symbol: (root.windowRef.controllerFullscreen
                || root.windowRef.visibility === Window.Maximized) ? "❐" : "□"
            onClicked: root.toggleWindowSize()
        }
        WindowControlButton {
            Layout.fillHeight: true
            Layout.preferredWidth: 42
            theme: root.theme
            symbol: "×"
            danger: true
            onClicked: root.windowRef.close()
        }
    }
}
