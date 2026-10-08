import QtQuick

Item {
    id: root

    property string name: "monitor"
    property color color: "#D8E0E6"
    property real strokeWidth: 1.7

    implicitWidth: 22
    implicitHeight: 22
    clip: true

    onNameChanged: canvas.requestPaint()
    onColorChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Component.onCompleted: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = root.color
            ctx.fillStyle = root.color
            ctx.lineWidth = root.strokeWidth
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
            function circle(cx, cy, r) {
                ctx.beginPath()
                ctx.arc(cx, cy, r, 0, Math.PI * 2)
                ctx.stroke()
            }
            function rect(x, y, rw, rh) {
                ctx.strokeRect(x, y, rw, rh)
            }

            if (root.name === "monitor") {
                rect(w*0.16, h*0.18, w*0.68, h*0.48)
                line(w*0.50, h*0.66, w*0.50, h*0.82)
                line(w*0.34, h*0.82, w*0.66, h*0.82)
            } else if (root.name === "mapping") {
                rect(w*0.16, h*0.16, w*0.24, h*0.24)
                rect(w*0.60, h*0.16, w*0.24, h*0.24)
                rect(w*0.16, h*0.60, w*0.24, h*0.24)
                rect(w*0.60, h*0.60, w*0.24, h*0.24)
            } else if (root.name === "hand") {
                ctx.beginPath()
                ctx.moveTo(w*0.36,h*0.82)
                ctx.lineTo(w*0.28,h*0.62)
                ctx.lineTo(w*0.22,h*0.44)
                ctx.lineTo(w*0.27,h*0.41)
                ctx.lineTo(w*0.38,h*0.57)
                ctx.lineTo(w*0.35,h*0.23)
                ctx.lineTo(w*0.40,h*0.20)
                ctx.lineTo(w*0.47,h*0.52)
                ctx.lineTo(w*0.48,h*0.14)
                ctx.lineTo(w*0.54,h*0.14)
                ctx.lineTo(w*0.58,h*0.51)
                ctx.lineTo(w*0.62,h*0.19)
                ctx.lineTo(w*0.68,h*0.20)
                ctx.lineTo(w*0.68,h*0.56)
                ctx.lineTo(w*0.74,h*0.32)
                ctx.lineTo(w*0.80,h*0.35)
                ctx.lineTo(w*0.76,h*0.64)
                ctx.quadraticCurveTo(w*0.71,h*0.85,w*0.52,h*0.87)
                ctx.quadraticCurveTo(w*0.41,h*0.87,w*0.36,h*0.82)
                ctx.stroke()
            } else if (root.name === "gamepad") {
                ctx.beginPath()
                ctx.moveTo(w*0.29,h*0.38)
                ctx.quadraticCurveTo(w*0.18,h*0.43,w*0.14,h*0.69)
                ctx.quadraticCurveTo(w*0.13,h*0.83,w*0.24,h*0.82)
                ctx.lineTo(w*0.38,h*0.67)
                ctx.lineTo(w*0.62,h*0.67)
                ctx.lineTo(w*0.76,h*0.82)
                ctx.quadraticCurveTo(w*0.87,h*0.83,w*0.86,h*0.69)
                ctx.quadraticCurveTo(w*0.82,h*0.43,w*0.71,h*0.38)
                ctx.quadraticCurveTo(w*0.62,h*0.34,w*0.50,h*0.39)
                ctx.quadraticCurveTo(w*0.38,h*0.34,w*0.29,h*0.38)
                ctx.stroke()
                line(w*0.27,h*0.53,w*0.39,h*0.53)
                line(w*0.33,h*0.47,w*0.33,h*0.59)
                circle(w*0.69,h*0.50,w*0.035)
                circle(w*0.76,h*0.57,w*0.035)
            } else if (root.name === "settings") {
                circle(w*0.50,h*0.50,w*0.16)
                for (let i = 0; i < 8; ++i) {
                    const a = i * Math.PI / 4
                    line(w*0.50+Math.cos(a)*w*0.25,h*0.50+Math.sin(a)*h*0.25,
                         w*0.50+Math.cos(a)*w*0.36,h*0.50+Math.sin(a)*h*0.36)
                }
                circle(w*0.50,h*0.50,w*0.34)
            } else if (root.name === "camera") {
                rect(w*0.17,h*0.31,w*0.66,h*0.46)
                circle(w*0.50,h*0.54,w*0.15)
                line(w*0.31,h*0.31,w*0.37,h*0.20)
                line(w*0.37,h*0.20,w*0.61,h*0.20)
                line(w*0.61,h*0.20,w*0.67,h*0.31)
            } else if (root.name === "fullscreen") {
                line(w*0.16,h*0.38,w*0.16,h*0.16)
                line(w*0.16,h*0.16,w*0.38,h*0.16)
                line(w*0.62,h*0.16,w*0.84,h*0.16)
                line(w*0.84,h*0.16,w*0.84,h*0.38)
                line(w*0.16,h*0.62,w*0.16,h*0.84)
                line(w*0.16,h*0.84,w*0.38,h*0.84)
                line(w*0.62,h*0.84,w*0.84,h*0.84)
                line(w*0.84,h*0.84,w*0.84,h*0.62)
            } else if (root.name === "target") {
                circle(w*0.50,h*0.50,w*0.28)
                circle(w*0.50,h*0.50,w*0.08)
                line(w*0.50,h*0.08,w*0.50,h*0.26)
                line(w*0.50,h*0.74,w*0.50,h*0.92)
                line(w*0.08,h*0.50,w*0.26,h*0.50)
                line(w*0.74,h*0.50,w*0.92,h*0.50)
            } else if (root.name === "play") {
                ctx.beginPath()
                ctx.moveTo(w*0.32,h*0.22)
                ctx.lineTo(w*0.78,h*0.50)
                ctx.lineTo(w*0.32,h*0.78)
                ctx.closePath()
                ctx.fill()
            } else if (root.name === "reset") {
                ctx.beginPath()
                ctx.arc(w*0.52,h*0.52,w*0.28,Math.PI*0.15,Math.PI*1.8)
                ctx.stroke()
                line(w*0.24,h*0.33,w*0.24,h*0.15)
                line(w*0.24,h*0.15,w*0.42,h*0.18)
            } else if (root.name === "info") {
                circle(w*0.50,h*0.50,w*0.34)
                circle(w*0.50,h*0.31,w*0.015)
                line(w*0.50,h*0.44,w*0.50,h*0.68)
            } else if (root.name === "user") {
                circle(w*0.50,h*0.34,w*0.15)
                ctx.beginPath()
                ctx.arc(w*0.50,h*0.78,w*0.27,Math.PI,Math.PI*2)
                ctx.stroke()
            } else if (root.name === "plus") {
                line(w*0.50,h*0.20,w*0.50,h*0.80)
                line(w*0.20,h*0.50,w*0.80,h*0.50)
            } else if (root.name === "trash") {
                line(w*0.28,h*0.30,w*0.34,h*0.82)
                line(w*0.72,h*0.30,w*0.66,h*0.82)
                line(w*0.34,h*0.82,w*0.66,h*0.82)
                line(w*0.24,h*0.28,w*0.76,h*0.28)
                line(w*0.39,h*0.20,w*0.61,h*0.20)
                line(w*0.43,h*0.40,w*0.43,h*0.70)
                line(w*0.57,h*0.40,w*0.57,h*0.70)
            } else if (root.name === "search") {
                circle(w*0.43,h*0.43,w*0.24)
                line(w*0.60,h*0.60,w*0.82,h*0.82)
            } else if (root.name === "save") {
                rect(w*0.18,h*0.14,w*0.64,h*0.72)
                rect(w*0.30,h*0.18,w*0.33,h*0.20)
                rect(w*0.30,h*0.58,w*0.40,h*0.22)
            } else if (root.name === "download") {
                line(w*0.50,h*0.14,w*0.50,h*0.62)
                line(w*0.34,h*0.47,w*0.50,h*0.64)
                line(w*0.66,h*0.47,w*0.50,h*0.64)
                line(w*0.20,h*0.82,w*0.80,h*0.82)
            } else if (root.name === "upload") {
                line(w*0.50,h*0.66,w*0.50,h*0.18)
                line(w*0.34,h*0.34,w*0.50,h*0.16)
                line(w*0.66,h*0.34,w*0.50,h*0.16)
                line(w*0.20,h*0.82,w*0.80,h*0.82)
            } else if (root.name === "menu") {
                line(w*0.18,h*0.30,w*0.82,h*0.30)
                line(w*0.18,h*0.50,w*0.82,h*0.50)
                line(w*0.18,h*0.70,w*0.82,h*0.70)
            } else {
                circle(w*0.50,h*0.50,w*0.30)
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }
}
