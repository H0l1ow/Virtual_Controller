import QtQuick

QtObject {
    // Overall palette: graphite/black surfaces + green status accent.
    readonly property color background: "#0A0F14"
    readonly property color backgroundAlt: "#0D1319"
    readonly property color topBar: "#0B1117"
    readonly property color surface: "#0E151C"
    readonly property color surfaceRaised: "#111A22"
    readonly property color surfaceHover: "#17212A"
    readonly property color surfacePressed: "#1B2630"
    readonly property color overlay: "#C90A1016"
    readonly property color overlaySoft: "#A90A1016"

    readonly property color border: "#2A3641"
    readonly property color borderStrong: "#394753"
    readonly property color borderSubtle: "#1A252E"

    readonly property color textPrimary: "#F1F5F7"
    readonly property color textSecondary: "#AEB9C3"
    readonly property color textMuted: "#75818D"
    readonly property color textFaint: "#55616C"

    readonly property color accent: "#34E6A1"
    readonly property color accentDim: "#0F5E45"
    readonly property color accentSoft: "#103329"
    readonly property color success: "#34E6A1"
    readonly property color warning: "#F0B45A"
    readonly property color error: "#F06C75"
    readonly property color neutral: "#8997A3"

    readonly property color leftHand: "#70E0C1"
    readonly property color rightHand: "#D5DDE4"

    // Slight rounding keeps the desktop/tool character while avoiding a dated,
    // completely square visual language.
    readonly property int cardRadius: 7
    readonly property int controlRadius: 5
    readonly property int smallRadius: 4
    readonly property int pageMargin: 16
    readonly property int topBarHeight: 72
    readonly property int borderWidth: 1

    // Typography
    readonly property string fontFamily: "Segoe UI"
    readonly property int fontHero: 26
    readonly property int fontTitle: 22
    readonly property int fontSection: 16
    readonly property int fontBody: 13
    readonly property int fontSmall: 11
    readonly property int fontTiny: 10
}
