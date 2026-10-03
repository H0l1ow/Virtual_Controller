import QtQuick

Item {
    id: root

    property string gesture: "open"
    property color color: "#E8EEF2"

    implicitWidth: 36
    implicitHeight: 36
    clip: true

    Canvas {
        id: canvas

        anchors.fill: parent
        anchors.margins: 2

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = root.color
            ctx.fillStyle = root.color
            ctx.lineWidth = Math.max(1.5, width / 22)
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            const w = width
            const h = height

            function line(x1, y1, x2, y2) {
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }

            function circle(x, y, radius) {
                ctx.beginPath()
                ctx.arc(x, y, radius, 0, Math.PI * 2)
                ctx.stroke()
            }

            if (root.gesture === "open") {
                ctx.beginPath()
                ctx.moveTo(w * 0.31, h * 0.80)
                ctx.lineTo(w * 0.24, h * 0.57)
                ctx.lineTo(w * 0.22, h * 0.38)
                ctx.lineTo(w * 0.29, h * 0.35)
                ctx.lineTo(w * 0.38, h * 0.54)
                ctx.lineTo(w * 0.35, h * 0.18)
                ctx.lineTo(w * 0.42, h * 0.17)
                ctx.lineTo(w * 0.48, h * 0.50)
                ctx.lineTo(w * 0.49, h * 0.11)
                ctx.lineTo(w * 0.56, h * 0.11)
                ctx.lineTo(w * 0.58, h * 0.50)
                ctx.lineTo(w * 0.63, h * 0.16)
                ctx.lineTo(w * 0.70, h * 0.18)
                ctx.lineTo(w * 0.68, h * 0.55)
                ctx.lineTo(w * 0.76, h * 0.31)
                ctx.lineTo(w * 0.82, h * 0.35)
                ctx.lineTo(w * 0.76, h * 0.66)
                ctx.quadraticCurveTo(w * 0.68, h * 0.87, w * 0.48, h * 0.87)
                ctx.quadraticCurveTo(w * 0.36, h * 0.87, w * 0.31, h * 0.80)
                ctx.stroke()
            } else if (root.gesture === "pinch") {
                ctx.beginPath()
                ctx.arc(w * 0.55, h * 0.44, w * 0.25, Math.PI * 0.65, Math.PI * 1.65)
                ctx.stroke()
                circle(w * 0.39, h * 0.31, w * 0.055)
                circle(w * 0.68, h * 0.39, w * 0.055)
                line(w * 0.42, h * 0.34, w * 0.55, h * 0.44)
                line(w * 0.65, h * 0.43, w * 0.56, h * 0.45)
                line(w * 0.48, h * 0.64, w * 0.34, h * 0.81)
                line(w * 0.56, h * 0.65, w * 0.70, h * 0.82)
            } else if (root.gesture === "point") {
                ctx.beginPath()
                ctx.moveTo(w * 0.31, h * 0.76)
                ctx.lineTo(w * 0.31, h * 0.50)
                ctx.lineTo(w * 0.45, h * 0.50)
                ctx.lineTo(w * 0.48, h * 0.18)
                ctx.lineTo(w * 0.56, h * 0.18)
                ctx.lineTo(w * 0.59, h * 0.50)
                ctx.lineTo(w * 0.73, h * 0.54)
                ctx.lineTo(w * 0.76, h * 0.63)
                ctx.lineTo(w * 0.67, h * 0.78)
                ctx.lineTo(w * 0.42, h * 0.80)
                ctx.closePath()
                ctx.stroke()
                line(w * 0.52, h * 0.18, w * 0.52, h * 0.06)
            } else if (root.gesture === "two") {
                line(w * 0.39, h * 0.76, w * 0.34, h * 0.25)
                line(w * 0.34, h * 0.25, w * 0.40, h * 0.20)
                line(w * 0.45, h * 0.56, w * 0.49, h * 0.16)
                line(w * 0.49, h * 0.16, w * 0.56, h * 0.18)
                line(w * 0.57, h * 0.58, w * 0.71, h * 0.38)
                line(w * 0.71, h * 0.38, w * 0.76, h * 0.43)
                ctx.beginPath()
                ctx.moveTo(w * 0.39, h * 0.76)
                ctx.quadraticCurveTo(w * 0.53, h * 0.86, w * 0.67, h * 0.73)
                ctx.lineTo(w * 0.76, h * 0.43)
                ctx.stroke()
            } else if (root.gesture === "fist") {
                ctx.strokeRect(w * 0.29, h * 0.36, w * 0.43, h * 0.38)
                line(w * 0.29, h * 0.48, w * 0.72, h * 0.48)
                line(w * 0.39, h * 0.36, w * 0.39, h * 0.55)
                line(w * 0.50, h * 0.36, w * 0.50, h * 0.55)
                line(w * 0.61, h * 0.36, w * 0.61, h * 0.55)
                line(w * 0.34, h * 0.74, w * 0.40, h * 0.86)
                line(w * 0.64, h * 0.74, w * 0.60, h * 0.86)
            } else if (root.gesture === "up" || root.gesture === "down") {
                const flip = root.gesture === "down"
                ctx.save()

                if (flip) {
                    ctx.translate(0, h)
                    ctx.scale(1, -1)
                }

                ctx.beginPath()
                ctx.moveTo(w * 0.30, h * 0.70)
                ctx.lineTo(w * 0.30, h * 0.49)
                ctx.lineTo(w * 0.46, h * 0.49)
                ctx.lineTo(w * 0.51, h * 0.18)
                ctx.lineTo(w * 0.59, h * 0.18)
                ctx.lineTo(w * 0.62, h * 0.48)
                ctx.lineTo(w * 0.76, h * 0.49)
                ctx.lineTo(w * 0.79, h * 0.58)
                ctx.lineTo(w * 0.70, h * 0.76)
                ctx.lineTo(w * 0.42, h * 0.76)
                ctx.closePath()
                ctx.stroke()
                ctx.restore()
            } else if (root.gesture === "swipe") {
                line(w * 0.18, h * 0.50, w * 0.82, h * 0.50)
                line(w * 0.18, h * 0.50, w * 0.32, h * 0.36)
                line(w * 0.18, h * 0.50, w * 0.32, h * 0.64)
                line(w * 0.82, h * 0.50, w * 0.68, h * 0.36)
                line(w * 0.82, h * 0.50, w * 0.68, h * 0.64)
            } else if (root.gesture === "rotate") {
                ctx.beginPath()
                ctx.arc(w * 0.50, h * 0.50, w * 0.28, Math.PI * 0.25, Math.PI * 1.55)
                ctx.stroke()
                line(w * 0.28, h * 0.29, w * 0.20, h * 0.45)
                line(w * 0.28, h * 0.29, w * 0.43, h * 0.31)
            } else {
                circle(w * 0.50, h * 0.50, w * 0.30)
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    onGestureChanged: canvas.requestPaint()
    onColorChanged: canvas.requestPaint()
}
