import QtQuick
import QtQml
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
        text: "CAMERA PREVIEW"
        color: root.theme.textPrimary
        opacity: 0.055
        font.family: root.theme.fontFamily
        font.pixelSize: 28
        font.weight: Font.Bold
        font.letterSpacing: 3
    }

    Canvas {
        id: handCanvas

        anchors.fill: parent
        visible: root.uiState.cameraRunning && root.uiState.showLandmarks

        function drawHand(ctx, cx, cy, scale, mirror, lineColor, pointColor) {
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

            function bone(startIndex, endIndex) {
                ctx.beginPath()
                ctx.moveTo(pointX(startIndex), pointY(startIndex))
                ctx.lineTo(pointX(endIndex), pointY(endIndex))
                ctx.stroke()
            }

            const bones = [
                [0, 1], [1, 2], [2, 3], [3, 4],
                [0, 5], [5, 6], [6, 7], [7, 8],
                [5, 9], [9, 10], [10, 11], [11, 12],
                [9, 13], [13, 14], [14, 15], [15, 16],
                [13, 17], [17, 18], [18, 19], [19, 20],
                [0, 17]
            ]

            ctx.strokeStyle = lineColor
            ctx.lineWidth = 1.5

            for (let i = 0; i < bones.length; ++i) {
                bone(bones[i][0], bones[i][1])
            }

            for (let j = 0; j < points.length; ++j) {
                ctx.fillStyle = "#0D141A"
                ctx.strokeStyle = pointColor
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(pointX(j), pointY(j), 3.3, 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()
            }
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const scale = Math.min(width, height) * 0.30
            const leftX = root.uiState.mirrorPreview ? 0.62 : 0.38
            const rightX = root.uiState.mirrorPreview ? 0.40 : 0.60
            if (root.uiState.leftTracked) {
                drawHand(
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
                drawHand(
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
                active: modelData.active && root.uiState.pipelineRunning
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
                    ? "L  " + root.uiState.leftGesture + "  "
                        + Math.round(root.uiState.leftConfidence * 100) + "%"
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
                    ? "R  " + root.uiState.rightGesture + "  "
                        + Math.round(root.uiState.rightConfidence * 100) + "%"
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
    Connections {
        target: root.uiState

        function onLeftTrackedChanged() {
            handCanvas.requestPaint()
        }

        function onRightTrackedChanged() {
            handCanvas.requestPaint()
        }

        function onShowLandmarksChanged() {
            handCanvas.requestPaint()
        }

        function onMirrorPreviewChanged() {
            handCanvas.requestPaint()
        }
    }

}
