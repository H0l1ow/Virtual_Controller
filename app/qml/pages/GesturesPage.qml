import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState

    property int liveHandIndex: 1
    readonly property bool liveHandIsLeft: liveHandIndex === 0
    readonly property bool liveHandTracked: liveHandIsLeft
        ? uiState.leftTracked
        : uiState.rightTracked
    readonly property string liveGesture: liveHandIsLeft
        ? uiState.leftGesture
        : uiState.rightGesture
    readonly property real liveConfidence: liveHandIsLeft
        ? uiState.leftConfidence
        : uiState.rightConfidence

    function glyphForGesture(gestureName) {
        switch (gestureName) {
        case "PINCH":
            return "pinch"
        case "POINT":
            return "point"
        case "FIST":
            return "fist"
        case "THUMB UP":
            return "up"
        case "THUMB DOWN":
            return "down"
        case "OPEN HAND":
            return "open"
        default:
            return "open"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Gestures"
            subtitle: "Available gestures, recognition preview and future custom gesture training."

            VcTextField {
                theme: root.theme
                placeholderText: "Search gestures (planned)"
                implicitWidth: 210
                enabled: false
            }

            VcButton {
                theme: root.theme
                text: "New gesture"
                iconName: "plus"
                implicitWidth: 120
                enabled: false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        Layout.leftMargin: 14
                        Layout.rightMargin: 14
                        spacing: 7

                        Text {
                            Layout.fillWidth: true
                            text: "Gesture Library"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        Repeater {
                            model: ["All", "Static", "Dynamic", "Two-hand"]

                            delegate: Rectangle {
                                required property string modelData

                                width: chipText.implicitWidth + 18
                                height: 30
                                color: modelData === "All"
                                    ? root.theme.surfaceHover
                                    : "transparent"
                                border.width: 1
                                border.color: modelData === "All"
                                    ? root.theme.borderStrong
                                    : root.theme.border
                                radius: root.theme.controlRadius

                                Text {
                                    id: chipText

                                    anchors.centerIn: parent
                                    text: modelData
                                    color: root.theme.textSecondary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    ScrollView {
                        id: gestureScroll

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth

                        Item {
                            width: gestureScroll.availableWidth
                            height: implicitHeight
                            implicitHeight: gestureGrid.implicitHeight + 24

                            GridLayout {
                                id: gestureGrid

                                x: 12
                                y: 12
                                width: parent.width - 24
                                columns: width >= 900 ? 3 : 2
                                columnSpacing: 10
                                rowSpacing: 10

                                Repeater {
                                    model: root.uiState.gestureLibrary

                                    delegate: Rectangle {
                                        required property var modelData

                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 184
                                        Layout.minimumHeight: 184
                                        color: root.theme.backgroundAlt
                                        border.width: 1
                                        border.color: modelData.enabled
                                            ? root.theme.border
                                            : root.theme.borderSubtle
                                        opacity: modelData.enabled ? 1.0 : 0.65
                                        radius: root.theme.cardRadius
                                        clip: true

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 13
                                            spacing: 7

                                            RowLayout {
                                                Layout.fillWidth: true

                                                GestureGlyph {
                                                    Layout.preferredWidth: 38
                                                    Layout.preferredHeight: 38
                                                    gesture: modelData.gesture
                                                    color: root.theme.textPrimary
                                                }

                                                Item {
                                                    Layout.fillWidth: true
                                                }

                                                TagChip {
                                                    theme: root.theme
                                                    text: modelData.category
                                                }
                                            }

                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.title
                                                color: root.theme.textPrimary
                                                font.family: root.theme.fontFamily
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.description
                                                color: root.theme.textMuted
                                                font.family: root.theme.fontFamily
                                                font.pixelSize: 10
                                                wrapMode: Text.WordWrap
                                                maximumLineCount: 2
                                                elide: Text.ElideRight
                                            }

                                            Item {
                                                Layout.fillHeight: true
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true

                                                StatusBadge {
                                                    theme: root.theme
                                                    text: modelData.enabled
                                                        ? "Available"
                                                        : "Planned"
                                                    kind: modelData.enabled
                                                        ? "success"
                                                        : "neutral"
                                                }

                                                Item {
                                                    Layout.fillWidth: true
                                                }

                                                Text {
                                                    text: "View details  ›"
                                                    color: root.theme.textSecondary
                                                    font.family: root.theme.fontFamily
                                                    font.pixelSize: 10
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            VcCard {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 320
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "camera"
                        title: "Live Recognition"
                        statusText: !root.uiState.gestureRecognitionAvailable
                            ? "M3: disabled"
                            : (!root.uiState.pipelineRunning
                                ? "Off"
                                : root.liveHandTracked ? "Live" : "Hand lost")
                        statusKind: !root.uiState.gestureRecognitionAvailable
                            ? "neutral"
                            : (!root.uiState.pipelineRunning
                                ? "neutral"
                                : root.liveHandTracked ? "success" : "warning")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 210
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        Layout.topMargin: 12
                        color: "#151D23"
                        border.width: 1
                        border.color: root.theme.borderStrong
                        clip: true

                        GestureGlyph {
                            anchors.centerIn: parent
                            width: 96
                            height: 96
                            visible: root.liveHandTracked
                                && root.uiState.gestureRecognitionAvailable
                            gesture: root.glyphForGesture(root.liveGesture)
                            color: root.theme.textPrimary
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: !root.liveHandTracked
                                || !root.uiState.gestureRecognitionAvailable
                            text: !root.uiState.gestureRecognitionAvailable
                                ? "GESTURE RECOGNITION STARTS IN M3"
                                : (root.uiState.pipelineRunning
                                    ? "HAND NOT TRACKED"
                                    : "PIPELINE STOPPED")
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.8
                        }

                        VcComboBox {
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.rightMargin: 10
                            anchors.topMargin: 10
                            theme: root.theme
                            model: ["Left hand", "Right hand"]
                            currentIndex: root.liveHandIndex
                            implicitWidth: 132
                            onActivated: root.liveHandIndex = currentIndex
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 10
                            anchors.bottomMargin: 10
                            text: root.liveHandIsLeft ? "LEFT HAND" : "RIGHT HAND"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 1
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.leftMargin: 14
                        Layout.rightMargin: 14
                        Layout.topMargin: 12
                        Layout.bottomMargin: 14
                        spacing: 10

                        Text {
                            text: "Detected gesture"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }

                        Text {
                            text: root.uiState.gestureRecognitionAvailable
                                ? root.liveGesture
                                : "Not implemented yet (M3)"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: "Handedness score"
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                            }

                            Text {
                                text: root.uiState.gestureRecognitionAvailable
                                    ? Math.round(root.liveConfidence * 100) + "%"
                                    : "--"
                                color: root.liveHandTracked
                                    && root.uiState.gestureRecognitionAvailable
                                    ? root.theme.accent
                                    : root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6
                            color: root.theme.borderStrong

                            Rectangle {
                                width: root.uiState.gestureRecognitionAvailable
                                    ? parent.width * root.liveConfidence
                                    : 0
                                height: parent.height
                                color: root.liveHandTracked
                                    && root.uiState.gestureRecognitionAvailable
                                    ? root.theme.accent
                                    : root.theme.textMuted
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.theme.borderSubtle
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Custom gesture recording and training is intentionally "
                                + "reserved for a later project stage."
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }

                        Item {
                            Layout.fillHeight: true
                        }

                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: "Record samples"
                            iconName: "camera"
                            enabled: false
                        }
                    }
                }
            }
        }
    }
}
