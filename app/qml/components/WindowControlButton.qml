import QtQuick

Rectangle {
    id: root
    required property QtObject theme
    property string symbol: "—"
    property bool danger: false
    signal clicked()

    implicitWidth: 48
    implicitHeight: 72
    color: mouse.containsMouse ? (danger ? "#A63D46" : theme.surfaceHover) : theme.topBar

    Text {
        anchors.centerIn: parent
        text: root.symbol
        color: root.theme.textPrimary
        font.family: root.theme.fontFamily
        font.pixelSize: root.symbol === "×" ? 24 : 18
        font.weight: Font.Light
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
