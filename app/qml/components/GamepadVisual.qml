import QtQuick

Item {
    id: root

    required property QtObject theme

    implicitWidth: 720
    implicitHeight: 440
    clip: true

    readonly property real scaleFactor: Math.min(width / 720, height / 440)

    // Controller shell. All interactive elements below have explicit z-order,
    // so nothing can accidentally render under a later panel or label.
    Canvas {
        id: bodyCanvas

        anchors.fill: parent
        z: 0
        opacity: 0.98

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const w = width
            const h = height

            ctx.fillStyle = "#101820"
            ctx.strokeStyle = root.theme.borderStrong
            ctx.lineWidth = Math.max(1, 1.6 * root.scaleFactor)
            ctx.lineJoin = "round"

            ctx.beginPath()
            ctx.moveTo(w * 0.28, h * 0.25)
            ctx.quadraticCurveTo(w * 0.17, h * 0.27, w * 0.12, h * 0.48)
            ctx.quadraticCurveTo(w * 0.07, h * 0.69, w * 0.09, h * 0.79)
            ctx.quadraticCurveTo(w * 0.11, h * 0.90, w * 0.20, h * 0.84)
            ctx.lineTo(w * 0.35, h * 0.69)
            ctx.quadraticCurveTo(w * 0.50, h * 0.76, w * 0.65, h * 0.69)
            ctx.lineTo(w * 0.80, h * 0.84)
            ctx.quadraticCurveTo(w * 0.89, h * 0.90, w * 0.91, h * 0.79)
            ctx.quadraticCurveTo(w * 0.93, h * 0.69, w * 0.88, h * 0.48)
            ctx.quadraticCurveTo(w * 0.83, h * 0.27, w * 0.72, h * 0.25)
            ctx.quadraticCurveTo(w * 0.61, h * 0.22, w * 0.50, h * 0.30)
            ctx.quadraticCurveTo(w * 0.39, h * 0.22, w * 0.28, h * 0.25)
            ctx.closePath()
            ctx.fill()
            ctx.stroke()

            // A small inner contour adds depth without introducing another icon.
            ctx.strokeStyle = root.theme.borderSubtle
            ctx.lineWidth = Math.max(1, root.scaleFactor)
            ctx.beginPath()
            ctx.moveTo(w * 0.33, h * 0.31)
            ctx.quadraticCurveTo(w * 0.50, h * 0.25, w * 0.67, h * 0.31)
            ctx.stroke()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Component.onCompleted: requestPaint()
    }

    // Shoulder buttons are deliberately kept inside the controller bounds.
    Rectangle {
        z: 1
        width: 122 * root.scaleFactor
        height: 30 * root.scaleFactor
        x: root.width * 0.25 - width / 2
        y: root.height * 0.245
        radius: 7 * root.scaleFactor
        color: root.theme.surfaceRaised
        border.width: 1
        border.color: root.theme.borderStrong

        Text {
            anchors.centerIn: parent
            text: "LB"
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: Math.max(8, 10 * root.scaleFactor)
            font.weight: Font.DemiBold
        }
    }

    Rectangle {
        z: 1
        width: 122 * root.scaleFactor
        height: 30 * root.scaleFactor
        x: root.width * 0.75 - width / 2
        y: root.height * 0.245
        radius: 7 * root.scaleFactor
        color: root.theme.surfaceRaised
        border.width: 1
        border.color: root.theme.borderStrong

        Text {
            anchors.centerIn: parent
            text: "RB"
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: Math.max(8, 10 * root.scaleFactor)
            font.weight: Font.DemiBold
        }
    }

    Canvas {
        id: dpadCanvas

        z: 2
        width: 88 * root.scaleFactor
        height: 88 * root.scaleFactor
        x: root.width * 0.25 - width / 2
        y: root.height * 0.49 - height / 2

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = root.theme.surfaceRaised
            ctx.strokeStyle = root.theme.borderStrong
            ctx.lineWidth = Math.max(1, root.scaleFactor)
            ctx.lineJoin = "round"

            const w = width
            const h = height
            const arm = w * 0.32
            const corner = 3 * root.scaleFactor

            // One single path avoids the old crossed-rectangle overlap artifact.
            ctx.beginPath()
            ctx.moveTo((w - arm) / 2 + corner, 0)
            ctx.lineTo((w + arm) / 2 - corner, 0)
            ctx.quadraticCurveTo((w + arm) / 2, 0, (w + arm) / 2, corner)
            ctx.lineTo((w + arm) / 2, (h - arm) / 2)
            ctx.lineTo(w - corner, (h - arm) / 2)
            ctx.quadraticCurveTo(w, (h - arm) / 2, w, (h - arm) / 2 + corner)
            ctx.lineTo(w, (h + arm) / 2 - corner)
            ctx.quadraticCurveTo(w, (h + arm) / 2, w - corner, (h + arm) / 2)
            ctx.lineTo((w + arm) / 2, (h + arm) / 2)
            ctx.lineTo((w + arm) / 2, h - corner)
            ctx.quadraticCurveTo((w + arm) / 2, h, (w + arm) / 2 - corner, h)
            ctx.lineTo((w - arm) / 2 + corner, h)
            ctx.quadraticCurveTo((w - arm) / 2, h, (w - arm) / 2, h - corner)
            ctx.lineTo((w - arm) / 2, (h + arm) / 2)
            ctx.lineTo(corner, (h + arm) / 2)
            ctx.quadraticCurveTo(0, (h + arm) / 2, 0, (h + arm) / 2 - corner)
            ctx.lineTo(0, (h - arm) / 2 + corner)
            ctx.quadraticCurveTo(0, (h - arm) / 2, corner, (h - arm) / 2)
            ctx.lineTo((w - arm) / 2, (h - arm) / 2)
            ctx.lineTo((w - arm) / 2, corner)
            ctx.quadraticCurveTo((w - arm) / 2, 0, (w - arm) / 2 + corner, 0)
            ctx.closePath()
            ctx.fill()
            ctx.stroke()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Component.onCompleted: requestPaint()
    }

    Item {
        id: faceButtons

        z: 2
        width: 106 * root.scaleFactor
        height: 106 * root.scaleFactor
        x: root.width * 0.75 - width / 2
        y: root.height * 0.47 - height / 2

        Repeater {
            model: [
                {
                    x: 0.50,
                    y: 0.12,
                    title: "Y"
                },
                {
                    x: 0.88,
                    y: 0.50,
                    title: "B"
                },
                {
                    x: 0.50,
                    y: 0.88,
                    title: "A"
                },
                {
                    x: 0.12,
                    y: 0.50,
                    title: "X"
                }
            ]

            delegate: Rectangle {
                required property var modelData

                width: 30 * root.scaleFactor
                height: width
                radius: width / 2
                x: faceButtons.width * modelData.x - width / 2
                y: faceButtons.height * modelData.y - height / 2
                color: root.theme.backgroundAlt
                border.width: 1
                border.color: root.theme.borderStrong

                Text {
                    anchors.centerIn: parent
                    text: modelData.title
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: Math.max(8, 11 * root.scaleFactor)
                    font.weight: Font.Bold
                }
            }
        }
    }

    Repeater {
        model: [
            {
                x: 0.38,
                y: 0.69,
                label: "L"
            },
            {
                x: 0.62,
                y: 0.69,
                label: "R"
            }
        ]

        delegate: Rectangle {
            required property var modelData

            z: 2
            width: 72 * root.scaleFactor
            height: width
            radius: width / 2
            x: root.width * modelData.x - width / 2
            y: root.height * modelData.y - height / 2
            color: root.theme.backgroundAlt
            border.width: 1
            border.color: root.theme.borderStrong

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.64
                height: width
                radius: width / 2
                color: root.theme.surfaceRaised
                border.width: 1
                border.color: root.theme.textFaint
            }

            Text {
                anchors.centerIn: parent
                text: modelData.label
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: Math.max(8, 10 * root.scaleFactor)
                font.weight: Font.Bold
            }
        }
    }

    Row {
        z: 3
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height * 0.40
        spacing: 14 * root.scaleFactor

        Rectangle {
            width: 30 * root.scaleFactor
            height: 20 * root.scaleFactor
            radius: 5 * root.scaleFactor
            color: root.theme.surfaceRaised
            border.width: 1
            border.color: root.theme.borderStrong

            Text {
                anchors.centerIn: parent
                text: "−"
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: Math.max(8, 11 * root.scaleFactor)
            }
        }

        Rectangle {
            width: 30 * root.scaleFactor
            height: 20 * root.scaleFactor
            radius: 5 * root.scaleFactor
            color: root.theme.surfaceRaised
            border.width: 1
            border.color: root.theme.borderStrong

            Text {
                anchors.centerIn: parent
                text: "≡"
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: Math.max(8, 11 * root.scaleFactor)
            }
        }
    }
}
