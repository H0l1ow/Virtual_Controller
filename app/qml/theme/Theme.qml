import QtQuick

QtObject {
    property string mode: "dark" // resolved dark/light from AppPreferences
    readonly property bool isLight: mode === "light"

    // Overall palette: graphite/black surfaces + green status accent.
    readonly property color background: isLight ? "#F3F6F8" : "#0A0F14"
    readonly property color backgroundAlt: isLight ? "#EBF1F4" : "#0D1319"
    readonly property color topBar: isLight ? "#FFFFFF" : "#0B1117"
    readonly property color surface: isLight ? "#FFFFFF" : "#0E151C"
    readonly property color surfaceRaised: isLight ? "#EFF3F6" : "#111A22"
    readonly property color surfaceHover: isLight ? "#E3EAEE" : "#17212A"
    readonly property color surfacePressed: isLight ? "#D7E1E7" : "#1B2630"
    readonly property color overlay: isLight ? "#DDEAF0F4" : "#C90A1016"
    readonly property color overlaySoft: isLight ? "#BEEAF0F4" : "#A90A1016"

    readonly property color border: isLight ? "#CFD9E0" : "#2A3641"
    readonly property color borderStrong: isLight ? "#AEBEC8" : "#394753"
    readonly property color borderSubtle: isLight ? "#DDE5E9" : "#1A252E"

    readonly property color textPrimary: isLight ? "#162631" : "#F1F5F7"
    readonly property color textSecondary: isLight ? "#465965" : "#AEB9C3"
    readonly property color textMuted: isLight ? "#637580" : "#75818D"
    readonly property color textFaint: isLight ? "#788A95" : "#55616C"

    readonly property color accent: isLight ? "#07865E" : "#34E6A1"
    readonly property color accentDim: isLight ? "#A6D5C2" : "#0F5E45"
    readonly property color accentSoft: isLight ? "#DCF1E8" : "#103329"
    readonly property color success: isLight ? "#07865E" : "#34E6A1"
    readonly property color warning: isLight ? "#9B620B" : "#F0B45A"
    readonly property color error: isLight ? "#C43F52" : "#F06C75"
    readonly property color neutral: isLight ? "#627683" : "#8997A3"

    readonly property color leftHand: isLight ? "#048A83" : "#70E0C1"
    readonly property color rightHand: isLight ? "#49617A" : "#D5DDE4"

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
