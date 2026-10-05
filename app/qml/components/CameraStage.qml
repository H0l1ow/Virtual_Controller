import QtQuick
import QtQml
import QtMultimedia
import QtQuick.Layouts

VcCard {
    id: root

    required property QtObject uiState

    clip: true
    color: "#151C22"

    Rectangle {
        anchors.fill: parent
        radius: root.theme.cardRadius

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#222C34"
            }

            GradientStop {
                position: 0.55
                color: "#182128"
            }

            GradientStop {
                position: 1.0
                color: "#11181E"
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: !root.uiState.cameraRunning
        text: "CAMERA PREVIEW"
        color: root.theme.textPrimary
        opacity: 0.055
        font.family: root.theme.fontFamily
        font.pixelSize: 28
        font.weight: Font.Bold
        font.letterSpacing: 3
    }

    VideoOutput {
        id: videoOutput

        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: root.uiState.cameraRunning
        mirrored: root.uiState.mirrorPreview

        Component.onCompleted: {
            root.uiState.attachVideoOutput(videoOutput)
        }

        Component.onDestruction: {
            root.uiState.attachVideoOutput(null)
        }
    }

    Canvas {
        id: handCanvas

        anchors.fill: parent
        visible: root.uiState.cameraRunning && root.uiState.showLandmarks

        readonly property var bones: [
            [0, 1], [1, 2], [2, 3], [3, 4],
            [0, 5], [5, 6], [6, 7], [7, 8],
            [5, 9], [9, 10], [10, 11], [11, 12],
            [9, 13], [13, 14], [14, 15], [15, 16],
            [13, 17], [17, 18], [18, 19], [19, 20],
            [0, 17]
        ]

        function drawTrackedHand(ctx, landmarks, lineColor, pointColor) {
            if (!landmarks || landmarks.length !== 21)
                return

            const rect = videoOutput.contentRect
            if (rect.width <= 0 || rect.height <= 0)
                return

            function pointX(index) {
                let x = landmarks[index].x
                if (root.uiState.mirrorPreview)
                    x = 1.0 - x
                return rect.x + x * rect.width
            }

            function pointY(index) {
                return rect.y + landmarks[index].y * rect.height
            }

            ctx.strokeStyle = lineColor
            ctx.lineWidth = 1.5

            for (let i = 0; i < bones.length; ++i) {
                ctx.beginPath()
                ctx.moveTo(pointX(bones[i][0]), pointY(bones[i][0]))
                ctx.lineTo(pointX(bones[i][1]), pointY(bones[i][1]))
                ctx.stroke()
            }

            for (let i = 0; i < landmarks.length; ++i) {
                ctx.fillStyle = "#0D141A"
                ctx.strokeStyle = pointColor
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(pointX(i), pointY(i), 3.3, 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()
            }
        }

        // Retains the previous visual mock when the app is started with
        // --mock-ui. The normal M1 path draws real MediaPipe landmarks.
        function drawMockHand(ctx, cx, cy, scale, mirror, lineColor, pointColor) {
            const points = [
                [0.00, 0.34],
                [-0.18, 0.16],
                [-0.28, -0.03],
                [-0.34, -0.23],
                [-0.38, -0.42],
                [-0.10, 0.08],
                [-0.14, -0.18],
                [-0.14, -0.43],
                [-0.13, -0.65],
                [0.00, 0.05],
                [0.00, -0.24],
                [0.01, -0.52],
                [0.02, -0.77],
                [0.10, 0.10],
                [0.13, -0.16],
                [0.16, -0.41],
                [0.19, -0.62],
                [0.19, 0.18],
                [0.25, -0.02],
                [0.30, -0.21],
                [0.34, -0.37]
            ]

            function pointX(index) {
                const direction = mirror ? -1 : 1
                return cx + direction * points[index][0] * scale
            }

            function pointY(index) {
                return cy + points[index][1] * scale
            }

            ctx.strokeStyle = lineColor
            ctx.lineWidth = 1.5

            for (let i = 0; i < bones.length; ++i) {
                ctx.beginPath()
                ctx.moveTo(pointX(bones[i][0]), pointY(bones[i][0]))
                ctx.lineTo(pointX(bones[i][1]), pointY(bones[i][1]))
                ctx.stroke()
            }

            for (let i = 0; i < points.length; ++i) {
                ctx.fillStyle = "#0D141A"
                ctx.strokeStyle = pointColor
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(pointX(i), pointY(i), 3.3, 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()
            }
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const liveLandmarks = root.uiState.leftLandmarks.length === 21
                || root.uiState.rightLandmarks.length === 21

            if (liveLandmarks) {
                if (root.uiState.leftTracked) {
                    drawTrackedHand(
                        ctx,
                        root.uiState.leftLandmarks,
                        "#D8E4E7",
                        root.theme.leftHand
                    )
                }

                if (root.uiState.rightTracked) {
                    drawTrackedHand(
                        ctx,
                        root.uiState.rightLandmarks,
                        "#D8E4E7",
                        root.theme.accent
                    )
                }

                return
            }

            // --mock-ui path only.
            const scale = Math.min(width, height) * 0.30
            const leftX = root.uiState.mirrorPreview ? 0.62 : 0.38
            const rightX = root.uiState.mirrorPreview ? 0.40 : 0.60

            if (root.uiState.leftTracked) {
                drawMockHand(
                    ctx,
                    width * leftX,
                    height * 0.68,
                    scale,
                    root.uiState.mirrorPreview,
                    "#D8E4E7",
                    root.theme.leftHand
                )
            }

            if (root.uiState.rightTracked) {
                drawMockHand(
                    ctx,
                    width * rightX,
                    height * 0.68,
                    scale,
                    !root.uiState.mirrorPreview,
                    "#D8E4E7",
                    root.theme.accent
                )
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 14
        anchors.topMargin: 14

        width: 178
        height: 40
        color: Qt.rgba(
            root.theme.background.r,
            root.theme.background.g,
            root.theme.background.b,
            0.88
        )
        border.width: 1
        border.color: root.theme.borderStrong
        radius: root.theme.controlRadius

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 10
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 5
                color: root.uiState.cameraRunning
                    ? root.theme.accent
                    : root.theme.error
            }

            Text {
                Layout.fillWidth: true
                text: root.uiState.cameraRunning
                    ? "Camera Active"
                    : "Camera Off"
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            Text {
                text: "⌄"
                color: root.theme.textSecondary
                font.pixelSize: 16
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 14
        anchors.topMargin: 14
        spacing: 8

        VcIconButton {
            theme: root.theme
            iconName: "camera"
            enabled: false
        }

        VcIconButton {
            theme: root.theme
            iconName: "fullscreen"
            enabled: false
        }
    }

    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 14
        anchors.topMargin: 66
        anchors.bottomMargin: 50

        width: Math.min(230, parent.width * 0.20)
        spacing: 4

        Repeater {
            model: root.uiState.gestureHints

            delegate: GestureHintRow {
                required property var modelData

                width: parent.width
                theme: root.theme
                gesture: modelData.gesture
                title: modelData.title
                action: modelData.action
                active: modelData.active
                    && root.uiState.pipelineRunning
                    && root.uiState.gestureRecognitionAvailable
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 14
        anchors.bottomMargin: 14

        height: 32
        width: stats.implicitWidth + 22
        color: Qt.rgba(
            root.theme.background.r,
            root.theme.background.g,
            root.theme.background.b,
            0.88
        )
        border.width: 1
        border.color: root.theme.borderStrong
        radius: root.theme.controlRadius

        Row {
            id: stats

            anchors.centerIn: parent
            spacing: 10

            Text {
                text: "FPS  " + root.uiState.fps
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            Text {
                text: "|"
                color: root.theme.textMuted
                font.pixelSize: 11
            }

            Text {
                text: root.uiState.resolution
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        spacing: 6

        Rectangle {
            width: leftText.implicitWidth + 18
            height: 28
            color: Qt.rgba(
                root.theme.background.r,
                root.theme.background.g,
                root.theme.background.b,
                0.84
            )
            border.width: 1
            border.color: root.theme.borderStrong
            radius: root.theme.controlRadius

            Text {
                id: leftText

                anchors.centerIn: parent
                text: root.uiState.leftTracked
                    ? (root.uiState.gestureRecognitionAvailable
                        ? "L  " + root.uiState.leftGesture + "  "
                            + Math.round(root.uiState.leftConfidence * 100) + "%"
                        : "L  TRACKED  "
                            + Math.round(root.uiState.leftConfidence * 100) + "%")
                    : "L  NOT TRACKED"
                color: root.uiState.leftTracked
                    ? root.theme.textSecondary
                    : root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }

        Rectangle {
            width: rightText.implicitWidth + 18
            height: 28
            color: Qt.rgba(
                root.theme.background.r,
                root.theme.background.g,
                root.theme.background.b,
                0.84
            )
            border.width: 1
            border.color: root.theme.accentDim
            radius: root.theme.controlRadius

            Text {
                id: rightText

                anchors.centerIn: parent
                text: root.uiState.rightTracked
                    ? (root.uiState.gestureRecognitionAvailable
                        ? "R  " + root.uiState.rightGesture + "  "
                            + Math.round(root.uiState.rightConfidence * 100) + "%"
                        : "R  TRACKED  "
                            + Math.round(root.uiState.rightConfidence * 100) + "%")
                    : "R  NOT TRACKED"
                color: root.uiState.rightTracked
                    ? root.theme.accent
                    : root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 64

        visible: root.uiState.errorMessage.length > 0
        width: Math.min(parent.width - 40, errorText.implicitWidth + 28)
        height: errorText.implicitHeight + 18
        radius: root.theme.controlRadius
        color: Qt.rgba(
            root.theme.background.r,
            root.theme.background.g,
            root.theme.background.b,
            0.94
        )
        border.width: 1
        border.color: root.theme.error

        Text {
            id: errorText

            anchors.centerIn: parent
            width: parent.width - 20
            text: root.uiState.errorMessage
            color: root.theme.error
            font.family: root.theme.fontFamily
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Connections {
        target: root.uiState

        function onLeftTrackedChanged() {
            handCanvas.requestPaint()
        }

        function onRightTrackedChanged() {
            handCanvas.requestPaint()
        }

        function onLeftLandmarksChanged() {
            handCanvas.requestPaint()
        }

        function onRightLandmarksChanged() {
            handCanvas.requestPaint()
        }

        function onShowLandmarksChanged() {
            handCanvas.requestPaint()
        }

        function onMirrorPreviewChanged() {
            handCanvas.requestPaint()
        }
    }

    Connections {
        target: videoOutput

        function onContentRectChanged() {
            handCanvas.requestPaint()
        }
    }
}
