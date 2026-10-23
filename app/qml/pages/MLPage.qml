import VirtualController.I18n 1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property bool previewActive: true
    // The recording itself belongs to C++ and continues if this page is hidden;
    // only optional offline help panels are released.
    onPreviewActiveChanged: {
        if (!previewActive && offlineTools)
            offlineTools.expanded = false
    }

    FileDialog {
        id: replayFileDialog
        title: qsTr("Open landmark recording")
        nameFilters: [qsTr("Gesture recordings (*.jsonl)")]
        onAccepted: root.uiState.startGestureReplay(selectedFile.toString())
    }

    function localizedReplayStatus(value) {
        LanguageManager.language
        switch (value) {
        case "Rules playback (offline, output disabled)": return qsTr("Rules playback (offline, output disabled)")
        case "Playback complete": return qsTr("Playback complete")
        case "Playback stopped": return qsTr("Playback stopped")
        case "Playback stopped: invalid recording": return qsTr("Playback stopped: invalid recording")
        default: return value
        }
    }

    function localizedGesture(value) {
        LanguageManager.language
        return qsTr(value)
    }

    function localizedFrameCount(value) {
        const n = Math.max(0, Math.floor(Number(value) || 0))
        if (LanguageManager.language === "pl") {
            const few = n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14)
            return n + (n === 1 ? " klatka" : few ? " klatki" : " klatek")
        }
        return n + (n === 1 ? " frame" : " frames")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("ML")
            subtitle: qsTr("ONNX inference, training data and offline model evaluation.")
            StatusBadge {
                theme: root.theme
                text: root.uiState.gestureBackendName
                kind: root.uiState.gestureModelActive ? "success" : "neutral"
            }
        }

        ScrollView {
            id: mlScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            contentHeight: mlContents.implicitHeight + 12
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip: true

            ColumnLayout {
                id: mlContents
                x: 0
                y: 0
                width: Math.max(1, mlScroll.availableWidth)
                spacing: 12

                GridLayout {
                    Layout.fillWidth: true
                    columns: 1
                    columnSpacing: 12
                    rowSpacing: 12

                    RecordingCapturePanel {
                        previewActive: root.previewActive
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.columnSpan: 1
                        Layout.preferredHeight: root.width >= 880 ? 600 : 1050
                        theme: root.theme
                        uiState: root.uiState
                    }

                    RecordingFilesPanel {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.columnSpan: 1
                        Layout.preferredHeight: 335
                        theme: root.theme
                        uiState: root.uiState
                    }

                    VcCard {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.columnSpan: 1
                        Layout.preferredHeight: root.width >= 900 ? 196 : 255
                        theme: root.theme

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 15
                            spacing: 8
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                VcIcon {
                                    name: "ml"
                                    color: root.theme.accent
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 28
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3
                                    Text {
                                        text: qsTr("Recognition model")
                                        color: root.theme.textPrimary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 17
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.uiState.gestureModelStatus
                                        color: root.theme.textMuted
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }
                                StatusBadge {
                                    theme: root.theme
                                    text: root.uiState.gestureBackendName
                                    kind: root.uiState.gestureModelActive ? "success" : "neutral"
                                }
                                VcButton {
                                    theme: root.theme
                                    iconName: "reset"
                                    text: qsTr("Reload model")
                                    enabled: root.uiState.runtimeStateName === "Running"
                                    onClicked: root.uiState.reloadGestureModel()
                                    ToolTip.visible: hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: qsTr("Restart tracking and reload the model. Enabled system input pauses safely and resumes after tracking stabilizes.")
                                }
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Gesture model") + ": models/gesture_model.onnx"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Hand tracker") + ": models/hand_landmarker.task"
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("The runtime uses Rules unless a compatible ONNX TCN has been deployed.")
                                wrapMode: Text.WordWrap
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                            }
                        }
                    }

                    DisclosurePanel {
                        id: offlineTools
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.columnSpan: 1
                        theme: root.theme
                        title: qsTr("Offline tools")
                        description: qsTr("Gesture replay and local Python training instructions")
                        iconName: "ml"
                        bodyHeight: root.width >= 1100 ? 322 : 626
                        bodyComponent: Component {
                            GridLayout {
                                columns: root.width >= 1100 ? 2 : 1
                                columnSpacing: 12
                                rowSpacing: 12
                    VcCard {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: 285
                        theme: root.theme
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10
                            Text {
                                font.family: root.theme.fontFamily
                                text: qsTr("Gesture Replay (offline)")
                                color: root.theme.textPrimary
                                font.pixelSize: 17

                                font.weight: Font.DemiBold
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: qsTr("Replay saved landmark sequences with Rules without a camera or Windows input. Stop tracking first.")
                                wrapMode: Text.WordWrap
                                color: root.theme.textMuted
                                font.pixelSize: 11
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                VcButton {
                                    theme: root.theme
                                    iconName: "folder"
                                    text: qsTr("Open recording")
                                    enabled: !root.uiState.pipelineRunning && !root.uiState.replayActive
                                    onClicked: replayFileDialog.open()
                                }
                                VcButton {
                                    theme: root.theme
                                    iconName: "stop"
                                    text: qsTr("Stop replay")
                                    enabled: root.uiState.replayActive
                                    onClicked: root.uiState.stopGestureReplay()
                                }
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: root.localizedReplayStatus(root.uiState.replayStatus) + " | "
                                      + root.localizedFrameCount(root.uiState.replayFrames)
                                color: root.theme.textSecondary
                                wrapMode: Text.WordWrap
                                font.pixelSize: 11
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: qsTr("Left") + ": " + root.localizedGesture(root.uiState.replayLeftGesture)
                                      + " (" + root.uiState.replayLeftLabel + ")    "
                                      + qsTr("Right") + ": " + root.localizedGesture(root.uiState.replayRightGesture)
                                      + " (" + root.uiState.replayRightLabel + ")"
                                color: root.theme.textPrimary
                                wrapMode: Text.WordWrap
                                font.pixelSize: 11
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                visible: root.uiState.replayError !== ""
                                text: root.uiState.replayError
                                wrapMode: Text.WordWrap
                                color: root.theme.error
                                font.pixelSize: 11
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                    VcCard {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: 285
                        theme: root.theme
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 11
                            Text {
                                font.family: root.theme.fontFamily
                                text: qsTr("Offline training & evaluation")
                                color: root.theme.textPrimary
                                font.pixelSize: 17

                                font.weight: Font.DemiBold
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: qsTr("Training runs in Python, outside Qt Creator. A trained ONNX model is not bundled.")
                                color: root.theme.textMuted
                                wrapMode: Text.WordWrap
                                font.pixelSize: 11
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: qsTr("0. Audit recordings") + "  ·  ml/scripts/audit_recordings.py\n"
                                    + qsTr("1. Prepare dataset") + "  ·  ml/scripts/prepare_dataset.py\n"
                                    + qsTr("2. Train TCN") + "  ·  ml/scripts/train.py\n"
                                    + qsTr("3. Evaluate TCN vs Rules") + "  ·  ml/scripts/evaluate.py\n"
                                    + qsTr("4. Export ONNX") + "  ·  ml/scripts/export_onnx.py"
                                color: root.theme.textPrimary

                                font.pixelSize: 12
                                lineHeight: 1.65
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                font.family: root.theme.fontFamily
                                Layout.fillWidth: true
                                text: qsTr("See ml/README.md for commands, leakage-safe dataset splits and model deployment.")
                                color: root.theme.textMuted
                                wrapMode: Text.WordWrap
                                font.pixelSize: 11
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                            }
                        }
                    }

                }
                Item { Layout.preferredHeight: 12 }
            }
        }
    }
}
