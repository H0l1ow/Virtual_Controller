import QtQuick
import QtQuick.Window
import QtMultimedia
import VirtualController.I18n 1.0
import VirtualController.Preferences 1.0

// A real independent tool window: it remains visible when the main window is
// minimized. The secondary QVideoSink receives the same capture frames used
// for MediaPipe, without opening a second camera.
Window {
    id: mini
    required property QtObject uiState
    required property QtObject theme
    property bool minimizeActive: false
    property bool enabledByUser: true
    property bool dismissed: false

    signal restoreRequested()

    // A separate Window does not receive a translated gesture label from the
    // runtime: keep translation inside the UI and track locale changes.
    function uiText(source) {
        // This also makes each binding depend on languageChanged, including
        // gesture labels that come from C++ as stable protocol identifiers.
        if (!LanguageManager.language)
            return source
        return qsTr(source)
    }

    // Size preset is independent of camera capture resolution and tracking.
    width: AppPreferences.pipSizeMode === "small" ? 304
        : AppPreferences.pipSizeMode === "large" ? 480 : 352
    height: AppPreferences.pipSizeMode === "small" ? 216
        : AppPreferences.pipSizeMode === "large" ? 340 : 250
    minimumWidth: 300
    minimumHeight: 210
    maximumWidth: 560
    maximumHeight: 400
    property bool placementReady: false
    visible: minimizeActive && enabledByUser && !dismissed
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    transientParent: null
    screen: AppPreferences.pipScreen
    onScreenChanged: {
        placementReady = false
        Qt.callLater(applySavedPosition)
    }
    color: "#0C151D"
    title: mini.uiText("Virtual Controller - Mini preview")

    function applySavedPosition() {
        if (!mini.visible || !mini.screen)
            return
        const bounds = mini.screen.availableGeometry
        if (!bounds)
            return
        const useSaved = AppPreferences.pipX !== -1 && AppPreferences.pipY !== -1
        const proposedX = useSaved ? AppPreferences.pipX : bounds.x + bounds.width - mini.width - 20
        const proposedY = useSaved ? AppPreferences.pipY : bounds.y + bounds.height - mini.height - 20
        // A monitor can be unplugged or its work area can shrink.
        mini.x = Math.max(bounds.x, Math.min(Math.round(proposedX),
                     bounds.x + Math.max(0, bounds.width - mini.width)))
        mini.y = Math.max(bounds.y, Math.min(Math.round(proposedY),
                     bounds.y + Math.max(0, bounds.height - mini.height)))
        mini.placementReady = true
    }

    onVisibleChanged: {
        uiState.setPiPActive(visible)
        if (visible) {
            placementReady = false
            Qt.callLater(applySavedPosition)
        } else {
            placementReady = false
        }
    }
    onXChanged: {
        if (visible && placementReady)
            AppPreferences.setPipPosition(mini.x, mini.y)
    }
    onYChanged: {
        if (visible && placementReady)
            AppPreferences.setPipPosition(mini.x, mini.y)
    }
    onWidthChanged: {
        if (visible)
            Qt.callLater(applySavedPosition)
    }
    onHeightChanged: {
        if (visible)
            Qt.callLater(applySavedPosition)
    }
    onMinimizeActiveChanged: {
        if (!minimizeActive)
            dismissed = false
    }
    onClosing: function(event) {
        event.accepted = false
        dismissed = true
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "#0C151D"
        border.width: 1
        border.color: "#35505C"
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 1
        height: 34
        color: "#14212B"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7
            Rectangle {
                width: 7; height: 7; radius: 4
                anchors.verticalCenter: parent.verticalCenter
                color: mini.uiState.pipelineRunning ? "#34E6A1" : "#F0B45A"
            }
            Text {
                text: mini.uiText("Live preview")
                anchors.verticalCenter: parent.verticalCenter
                color: "#F1F5F7"
                font.pixelSize: 12
                font.bold: true
            }
        }
        Text {
            anchors.right: closeControl.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            color: "#AEB9C3"
            font.pixelSize: 10
            text: mini.uiState.processedFps + " FPS"
        }
        Rectangle {
            id: closeControl
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            width: 26; height: 24; radius: 4
            color: closeMouse.containsMouse ? "#51313A" : "transparent"
            Text { anchors.centerIn: parent; text: "×"; color: "#F1F5F7"; font.pixelSize: 16 }
            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: mini.dismissed = true
            }
        }
        MouseArea {
            anchors.left: parent.left
            anchors.right: closeControl.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            acceptedButtons: Qt.LeftButton
            onPressed: mini.startSystemMove()
            onDoubleClicked: mini.restoreRequested()
        }
    }

    Item {
        id: previewArea
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true

        Rectangle { anchors.fill: parent; color: "#101B23" }
        VideoOutput {
            id: pipVideo
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
            mirrored: mini.uiState.mirrorPreview
            visible: mini.uiState.cameraRunning
            Component.onCompleted: mini.uiState.attachPiPVideoSink(videoSink)
            Component.onDestruction: mini.uiState.attachPiPVideoSink(null)
        }
        Text {
            anchors.centerIn: parent
            visible: !mini.uiState.cameraRunning
            color: "#AEB9C3"
            font.pixelSize: 13
            text: mini.uiText("Camera inactive")
        }

        Canvas {
            id: skeleton
            anchors.fill: parent
            visible: mini.uiState.cameraRunning && mini.uiState.showLandmarks
                     && AppPreferences.pipShowLandmarks
            readonly property var bones: [
                [0,1],[1,2],[2,3],[3,4],
                [0,5],[5,6],[6,7],[7,8],
                [5,9],[9,10],[10,11],[11,12],
                [9,13],[13,14],[14,15],[15,16],
                [0,17],[17,18],[18,19],[19,20]
            ]
            function drawHand(ctx, landmarks, ink) {
                if (!landmarks || landmarks.length !== 21)
                    return
                const area = pipVideo.contentRect
                if (area.width <= 0 || area.height <= 0)
                    return
                function xp(index) {
                    const value = landmarks[index].x
                    return area.x + (mini.uiState.mirrorPreview ? 1 - value : value) * area.width
                }
                function yp(index) { return area.y + landmarks[index].y * area.height }
                ctx.strokeStyle = ink
                ctx.fillStyle = ink
                ctx.lineWidth = 1.3
                for (let i = 0; i < bones.length; ++i) {
                    ctx.beginPath()
                    ctx.moveTo(xp(bones[i][0]), yp(bones[i][0]))
                    ctx.lineTo(xp(bones[i][1]), yp(bones[i][1]))
                    ctx.stroke()
                }
                for (let i = 0; i < 21; ++i) {
                    ctx.beginPath()
                    ctx.arc(xp(i), yp(i), 2.0, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (mini.uiState.leftTracked)
                    drawHand(ctx, mini.uiState.leftLandmarks, "#70E0C1")
                if (mini.uiState.rightTracked)
                    drawHand(ctx, mini.uiState.rightLandmarks, "#D5DDE4")
            }
            Connections {
                target: mini.uiState
                function onLeftLandmarksChanged() { skeleton.requestPaint() }
                function onRightLandmarksChanged() { skeleton.requestPaint() }
                function onMirrorPreviewChanged() { skeleton.requestPaint() }
            }
            Connections {
                target: pipVideo
                function onContentRectChanged() { skeleton.requestPaint() }
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: mini.restoreRequested()
            onPressed: function(mouse) {
                if (mouse.modifiers & Qt.ShiftModifier)
                    mini.startSystemMove()
            }
        }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        height: 33
        color: "#14212B"
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: mini.uiText("Left") + ": " + mini.uiText(mini.uiState.leftGesture || "NONE")
                  + "  |  " + mini.uiText("Right") + ": "
                  + mini.uiText(mini.uiState.rightGesture || "NONE")
            color: "#E1E9EC"
            font.pixelSize: 10
            elide: Text.ElideRight
            width: parent.width - 24
        }
    }
}
