import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import VirtualController.I18n 1.0

VcCard {
    id: root
    required property QtObject uiState
    property string pendingDeleteName: ""

    function prettySize(bytes) {
        const kb = Number(bytes) / 1024
        return kb < 1024 ? Math.round(kb) + " KB" : (kb / 1024).toFixed(1) + " MB"
    }

    function errorText(value) {
        LanguageManager.language
        switch (value) {
        case "Stop recording before changing the folder.": return qsTr("Stop recording before changing the folder.")
        case "Choose a local folder.": return qsTr("Choose a local folder.")
        case "Selected folder is not writable.": return qsTr("Selected folder is not writable.")
        case "Cannot create the recordings folder.": return qsTr("Cannot create the recordings folder.")
        case "Could not open the recordings folder.": return qsTr("Could not open the recordings folder.")
        case "Cannot open the selected recording.": return qsTr("Cannot open the selected recording.")
        case "Stop recording or replay before deleting files.": return qsTr("Stop recording or replay before deleting files.")
        case "Recording not found.": return qsTr("Recording not found.")
        default:
            if (value.startsWith("Cannot move the recording to Recycle Bin:"))
                return qsTr("Cannot move the recording to Recycle Bin:") + value.substring("Cannot move the recording to Recycle Bin:".length)
            return value
        }
    }

    FolderDialog {
        id: chooseFolder
        title: qsTr("Choose recordings folder")
        onAccepted: root.uiState.setRecordingsDirectory(selectedFolder)
    }
    Dialog {
        id: confirmDelete
        title: qsTr("Move recording to Recycle Bin?")
        modal: true
        width: Math.min(450, root.width - 40)
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.uiState.deleteRecordingFile(root.pendingDeleteName)
        contentItem: Text {
            text: qsTr("The selected JSONL file will be moved to the system Recycle Bin. Other sessions will not be affected.")
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("My recordings")
                font.family: root.theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                color: root.theme.textPrimary
                Layout.fillWidth: true
            }
            Text {
                text: root.uiState.recordings.length + " " + qsTr("files")
                color: root.theme.textMuted
                font.pixelSize: 11
            }
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Files are stored in your local app data folder by default, not in the source code or build directory. You can open or change this location below.")
            wrapMode: Text.WordWrap
            color: root.theme.textSecondary
            font.family: root.theme.fontFamily
            font.pixelSize: 11
        }
        Text {
            Layout.fillWidth: true
            text: root.uiState.recordingsDirectory
            wrapMode: Text.WrapAnywhere
            font.family: root.theme.fontFamily
            font.pixelSize: 11
            color: root.theme.textMuted
        }
        Flow {
            Layout.fillWidth: true
            spacing: 8
            VcButton {
                theme: root.theme
                iconName: "folder"
                text: qsTr("Open folder")
                onClicked: root.uiState.openRecordingsFolder()
            }
            VcButton {
                theme: root.theme
                text: qsTr("Change folder")
                enabled: !root.uiState.recordingActive
                onClicked: chooseFolder.open()
            }
            VcButton {
                theme: root.theme
                text: qsTr("Default folder")
                enabled: !root.uiState.recordingActive
                onClicked: root.uiState.resetRecordingsDirectory()
            }
            VcButton {
                theme: root.theme
                iconName: "reset"
                text: qsTr("Refresh list")
                onClicked: root.uiState.refreshRecordings()
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
        Text {
            Layout.fillWidth: true
            visible: root.uiState.recordings.length === 0
            text: qsTr("No recordings in this folder yet. Start tracking, choose a gesture, and record your first session above.")
            wrapMode: Text.WordWrap
            color: root.theme.textSecondary
            font.pixelSize: 12
            font.family: root.theme.fontFamily
        }
        ListView {
            id: recordingsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.uiState.recordings.length > 0
            clip: true
            spacing: 5
            model: root.uiState.recordings
            delegate: Rectangle {
                required property var modelData
                width: recordingsList.width
                height: 60
                radius: root.theme.smallRadius
                color: root.theme.surfaceRaised
                border.color: root.theme.borderSubtle
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            text: modelData.name
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                        }
                        Text {
                            text: modelData.modified + " · " + root.prettySize(modelData.size)
                            color: root.theme.textMuted
                            font.pixelSize: 10
                        }
                    }
                    VcButton {
                        theme: root.theme
                        text: qsTr("Open file")
                        enabled: !modelData.active
                        onClicked: root.uiState.openRecordingFile(modelData.name)
                    }
                    VcButton {
                        theme: root.theme
                        text: qsTr("Replay")
                        enabled: !root.uiState.pipelineRunning && !root.uiState.replayActive && !modelData.active
                        onClicked: root.uiState.replayRecordingFile(modelData.name)
                    }
                    VcButton {
                        theme: root.theme
                        variant: "danger"
                        text: qsTr("Delete")
                        enabled: !root.uiState.recordingActive && !root.uiState.replayActive
                        onClicked: {
                            root.pendingDeleteName = modelData.name
                            confirmDelete.open()
                        }
                    }
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: root.uiState.recordingsError !== ""
            text: root.errorText(root.uiState.recordingsError)
            color: root.theme.error
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Delete moves a file to Recycle Bin. The list shows up to 100 newest JSONL sessions from the selected folder.")
            color: root.theme.textMuted
            font.pixelSize: 10
            font.family: root.theme.fontFamily
            wrapMode: Text.WordWrap
        }
    }
}
