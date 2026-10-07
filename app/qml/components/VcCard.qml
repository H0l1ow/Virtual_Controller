import QtQuick

Rectangle {
    required property QtObject theme

    // Controller immersive mode can make card backgrounds translucent without
    // reducing text/icon opacity. Normal cards keep the original M1 appearance.
    property bool translucent: false
    property real translucentOpacity: 0.80

    color: translucent
        ? Qt.rgba(
            theme.surface.r,
            theme.surface.g,
            theme.surface.b,
            translucentOpacity
        )
        : theme.surface
    radius: theme.cardRadius
    border.width: 1
    border.color: theme.border
    clip: true
}
