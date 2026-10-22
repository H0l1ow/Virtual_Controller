import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import VirtualController.I18n 1.0

VcCard {
    id: root
    required property QtObject uiState
    property string leftRecordLabel: "UNLABELED"
    property string rightRecordLabel: "UNLABELED"
    // Pausing preview does not stop tracking or dataset recording.
    property bool previewEnabled: true
    property bool previewActive: true
    property bool previewReady: false
    readonly property bool renderingPreview: root.visible && root.previewActive
                                              && root.previewEnabled

    function updatePreviewAttachment() {
        if (!root.previewReady)
            return
        root.uiState.attachRecordingVideoSink(
            root.renderingPreview ? recordingVideo.videoSink : null)
    }

    onRenderingPreviewChanged: updatePreviewAttachment()
    readonly property var recordingLabels: ["UNLABELED", "NONE", "FIST", "OPEN_HAND", "POINT", "PINCH"]
    readonly property var quality: uiState.recordingQuality
    readonly property var leftQuality: quality.left || {labelled: false, usable: 0, missing: 0, percent: 0, trackedPercent: 0}
    readonly property var rightQuality: quality.right || {labelled: false, usable: 0, missing: 0, percent: 0, trackedPercent: 0}
    readonly property int targetFrames: quality.target || 150

    function displayLabel(value) {
        LanguageManager.language
        switch (value) {
        case "UNLABELED": return qsTr("Ignore this hand")
        case "NONE": return qsTr("NONE (neutral pose)")
        default: return value
        }
    }

    function errorText(value) {
        LanguageManager.language
        switch (value) {
        case "Select at least one labelled hand.": return qsTr("Select at least one labelled hand.")
        case "Start pipeline first; only one recording at a time": return qsTr("Start pipeline first; only one recording at a time")
        case "Invalid gesture label": return qsTr("Invalid gesture label")
        default: return value
        }
    }

    function stepHint() {
        if (!uiState.pipelineRunning)
            return qsTr("Start camera tracking in the top bar before recording.")
        if (uiState.recordingActive) {
            if ((leftQuality.labelled && !uiState.leftTracked)
                    || (rightQuality.labelled && !uiState.rightTracked))
                return qsTr("Keep the labelled hand visible and fully inside the camera view.")
            return qsTr("Recording: hold the selected pose, then try a slightly different angle or distance.")
        }
        if (leftRecordLabel === "UNLABELED" && rightRecordLabel === "UNLABELED")
            return qsTr("Select a gesture for at least one hand. The other hand can be ignored.")
        return qsTr("Ready. Check the camera preview and press Record samples.")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Record training samples")
                font.family: root.theme.fontFamily
                font.pixelSize: 17
                font.weight: Font.DemiBold
                color: root.theme.textPrimary
                Layout.fillWidth: true
            }
            StatusBadge {
                theme: root.theme
                text: root.uiState.recordingActive ? qsTr("Recording") : qsTr("Not recording")
                kind: root.uiState.recordingActive ? "success" : "neutral"
            }
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Record one gesture per session. Only hand landmarks and labels are saved; camera images and audio are never stored.")
            color: root.theme.textSecondary
            font.family: root.theme.fontFamily
            font.pixelSize: 12
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: root.width >= 880 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 12
            ColumnLayout {
                // On desktop, controls stay on the right of the live camera.
                Layout.column: root.width >= 880 ? 1 : 0
                Layout.row: root.width >= 880 ? 0 : 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                spacing: 10
                Text {
                    Layout.fillWidth: true
                    text: qsTr("1. Start tracking   2. Choose gesture labels   3. Record and check progress   4. Stop")
                    wrapMode: Text.WordWrap
                    color: root.theme.textPrimary
                    font.pixelSize: 12
                    font.family: root.theme.fontFamily
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("FIST = closed hand; OPEN_HAND = open palm; POINT = index finger; PINCH = thumb touching index finger; NONE = tracked hand in a neutral pose. Ignore = do not use this hand for training.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textMuted
                    font.pixelSize: 11
                    font.family: root.theme.fontFamily
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Text { text: qsTr("LEFT hand"); color: root.theme.textSecondary; font.pixelSize: 11 }
                        VcComboBox {
                            theme: root.theme
                            Layout.fillWidth: true
                            model: root.recordingLabels
                            enabled: !root.uiState.recordingActive
                            labelFormatter: root.displayLabel
                            onActivated: root.leftRecordLabel = currentText
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Text { text: qsTr("RIGHT hand"); color: root.theme.textSecondary; font.pixelSize: 11 }
                        VcComboBox {
                            theme: root.theme
                            Layout.fillWidth: true
                            model: root.recordingLabels
                            enabled: !root.uiState.recordingActive
                            labelFormatter: root.displayLabel
                            onActivated: root.rightRecordLabel = currentText
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: hintText.implicitHeight + 20
                    radius: root.theme.controlRadius
                    color: root.theme.surfaceRaised
                    border.color: root.theme.borderSubtle
                    Text {
                        id: hintText
                        anchors.fill: parent
                        anchors.margins: 10
                        wrapMode: Text.WordWrap
                        font.pixelSize: 11
                        font.family: root.theme.fontFamily
                        color: root.theme.textPrimary
                        text: root.stepHint()
                    }
                }
                Repeater {
                    model: ["LEFT", "RIGHT"]
                    delegate: ColumnLayout {
                        required property string modelData
                        readonly property var stats: modelData === "LEFT" ? root.leftQuality : root.rightQuality
                        readonly property bool detected: modelData === "LEFT" ? root.uiState.leftTracked : root.uiState.rightTracked
                        readonly property string chosenLabel: modelData === "LEFT" ? root.leftRecordLabel : root.rightRecordLabel
                        readonly property bool chosen: chosenLabel !== "UNLABELED"
                        readonly property bool relevant: stats.labelled && chosenLabel === stats.label
                        Layout.fillWidth: true
                        spacing: 3
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: (modelData === "LEFT" ? qsTr("Left hand") : qsTr("Right hand")) +
                                      " — " + (chosen ? (detected ? qsTr("Detected") : qsTr("Not detected")) : qsTr("Ignored"))
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                            }
                            Text {
                                text: relevant ? stats.usable + " / " + root.targetFrames : (chosen ? "0 / " + root.targetFrames : "—")
                                color: root.theme.textSecondary
                                font.pixelSize: 11
                            }
                        }
                        ProgressBar {
                            Layout.fillWidth: true
                            implicitHeight: 12
                            from: 0
                            to: 100
                            value: relevant ? stats.percent : 0
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: relevant && root.uiState.recordedFrames > 0
                            text: qsTr("Tracked frames") + ": " + stats.usable + " · "
                                  + qsTr("Missing/invalid") + ": " + stats.missing
                            color: root.theme.textMuted
                            font.pixelSize: 10
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    VcButton {
                        theme: root.theme
                        iconName: root.uiState.recordingActive ? "stop" : "camera"
                        text: root.uiState.recordingActive ? qsTr("Stop recording") : qsTr("Record samples")
                        enabled: root.uiState.recordingActive || (root.uiState.pipelineRunning
                                    && (root.leftRecordLabel !== "UNLABELED" || root.rightRecordLabel !== "UNLABELED"))
                        onClicked: {
                            if (root.uiState.recordingActive)
                                root.uiState.stopGestureRecording()
                            else
                                root.uiState.startGestureRecording(root.leftRecordLabel, root.rightRecordLabel)
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Saved frames") + ": " + root.uiState.recordedFrames
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("150 valid tracked frames per labelled hand is a suggested minimum, not an accuracy score. A full bar does not validate the gesture label. Capture multiple separate sessions with different angles and lighting.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textMuted
                    font.pixelSize: 10
                    font.family: root.theme.fontFamily
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.uiState.recordingError !== ""
                    text: root.errorText(root.uiState.recordingError)
                    color: root.theme.error
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                }
            }
            ColumnLayout {
                Layout.column: 0
                Layout.row: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Live camera preview")
                        font.family: root.theme.fontFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: root.theme.textPrimary
                    }
                    Text {
                        text: root.uiState.processedFps + " FPS"
                        font.pixelSize: 11
                        color: root.theme.textSecondary
                    }
                    VcButton {
                        theme: root.theme
                        text: root.previewEnabled ? qsTr("Pause preview") : qsTr("Resume preview")
                        onClicked: root.previewEnabled = !root.previewEnabled
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 180
                    color: "#121A22"
                    radius: root.theme.controlRadius
                    clip: true
                    VideoOutput {
                        id: recordingVideo
                        anchors.fill: parent
                        fillMode: VideoOutput.PreserveAspectFit
                        mirrored: root.uiState.mirrorPreview
                        visible: root.uiState.cameraRunning && root.renderingPreview
                        Component.onCompleted: {
                            root.previewReady = true
                            root.updatePreviewAttachment()
                        }
                        Component.onDestruction: {
                            root.previewReady = false
                            root.uiState.attachRecordingVideoSink(null)
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: !root.uiState.cameraRunning || !root.renderingPreview
                        text: root.previewEnabled
                              ? qsTr("Start tracking to see the camera")
                              : qsTr("Preview paused")
                        color: root.theme.textSecondary
                        font.pixelSize: 12
                    }
                    Canvas {
                        id: handDrawing
                        anchors.fill: parent
                        visible: root.uiState.cameraRunning && root.renderingPreview
                                 && root.uiState.showLandmarks
                        readonly property var bones: [
                            [0,1],[1,2],[2,3],[3,4], [0,5],[5,6],[6,7],[7,8],
                            [5,9],[9,10],[10,11],[11,12], [9,13],[13,14],[14,15],[15,16],
                            [0,17],[17,18],[18,19],[19,20]
                        ]
                        function drawHand(ctx, landmarks, color) {
                            if (!landmarks || landmarks.length !== 21)
                                return
                            const rect = recordingVideo.contentRect
                            if (rect.width <= 0 || rect.height <= 0) return
                            function px(i) { return rect.x + (root.uiState.mirrorPreview ?
                                (1 - landmarks[i].x) : landmarks[i].x) * rect.width }
                            function py(i) { return rect.y + landmarks[i].y * rect.height }
                            ctx.strokeStyle = color
                            ctx.fillStyle = color
                            ctx.lineWidth = 1.5
                            for (let i = 0; i < bones.length; ++i) {
                                ctx.beginPath()
                                ctx.moveTo(px(bones[i][0]), py(bones[i][0]))
                                ctx.lineTo(px(bones[i][1]), py(bones[i][1]))
                                ctx.stroke()
                            }
                            for (let i = 0; i < 21; ++i) {
                                ctx.beginPath()
                                ctx.arc(px(i), py(i), 2, 0, Math.PI * 2)
                                ctx.fill()
                            }
                        }
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            if (root.uiState.leftTracked)
                                drawHand(ctx, root.uiState.leftLandmarks, "#70E0C1")
                            if (root.uiState.rightTracked)
                                drawHand(ctx, root.uiState.rightLandmarks, "#D5DDE4")
                        }
                        // A 10 Hz skeleton overlay is sufficient for dataset
                        // positioning. Per-landmark repaint events (2 per frame)
                        // previously overloaded Qt Quick's rendering thread.
                        Timer {
                            interval: 100
                            repeat: true
                            running: root.renderingPreview
                                     && root.uiState.cameraRunning && root.uiState.showLandmarks
                            onTriggered: handDrawing.requestPaint()
                        }
                        Connections {
                            target: root.uiState
                            enabled: handDrawing.visible
                            function onMirrorPreviewChanged() { handDrawing.requestPaint() }
                        }
                        Connections {
                            target: recordingVideo
                            enabled: handDrawing.visible
                            function onContentRectChanged() { handDrawing.requestPaint() }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Preview only — camera images are not saved. The label is your annotation, not an ML prediction.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textMuted
                    font.pixelSize: 10
                    font.family: root.theme.fontFamily
                }
            }
        }
    }
    onVisibleChanged: updatePreviewAttachment()
}
