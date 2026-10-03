import QtQuick

Item {
    id: root
    property color color: "#DCE5EC"
    implicitWidth: 38
    implicitHeight: 38

    VcIcon {
        anchors.fill: parent
        name: "hand"
        color: root.color
        strokeWidth: 2.3
    }
}
