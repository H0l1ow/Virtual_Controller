import QtQuick

QtObject {
    property string mode: "dark" // resolved dark/light from AppPreferences
    readonly property bool isLight: mode === "light"

    // Overall palette: graphite/black surfaces + green status accent.
    readonly property color background: isLight ? "#F3F6F8" : "#070F1C"
    readonly property color backgroundAlt: isLight ? "#EBF1F4" : "#0A1525"
    readonly property color topBar: isLight ? "#FFFFFF" : "#07111F"
    readonly property color surface: isLight ? "#FFFFFF" : "#0D1B2C"
    readonly property color surfaceRaised: isLight ? "#EFF3F6" : "#11243A"
    readonly property color surfaceHover: isLight ? "#E3EAEE" : "#17304B"
    readonly property color surfacePressed: isLight ? "#D7E1E7" : "#1A3856"
    readonly property color overlay: isLight ? "#DDEAF0F4" : "#C90A1016"
    readonly property color overlaySoft: isLight ? "#BEEAF0F4" : "#A90A1016"

    readonly property color border: isLight ? "#CFD9E0" : "#223B54"
    readonly property color borderStrong: isLight ? "#AEBEC8" : "#31506D"
    readonly property color borderSubtle: isLight ? "#DDE5E9" : "#1A3047"

    readonly property color textPrimary: isLight ? "#162631" : "#F1F5F7"
    readonly property color textSecondary: isLight ? "#465965" : "#AEB9C3"
    readonly property color textMuted: isLight ? "#637580" : "#75818D"
    readonly property color textFaint: isLight ? "#788A95" : "#55616C"

    readonly property color accent: isLight ? "#087DB4" : "#20BCFF"
    readonly property color accentDim: isLight ? "#9ACCE1" : "#1B6693"
    readonly property color accentSoft: isLight ? "#E4F4FA" : "#0A3154"
    readonly property color success: isLight ? "#07865E" : "#34E6A1"
    readonly property color warning: isLight ? "#9B620B" : "#F0B45A"
    readonly property color error: isLight ? "#C43F52" : "#F06C75"
    readonly property color neutral: isLight ? "#627683" : "#8997A3"

    readonly property color leftHand: isLight ? "#048A83" : "#70E0C1"
    readonly property color rightHand: isLight ? "#49617A" : "#5EA9FF"

    // Slight rounding keeps the desktop/tool character while avoiding a dated,
    // completely square visual language.
    readonly property int cardRadius: 14
    readonly property int controlRadius: 10
    readonly property int smallRadius: 6
    readonly property int pageMargin: 18
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
