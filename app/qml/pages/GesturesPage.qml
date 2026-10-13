import VirtualController.I18n 1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState

    property int liveHandIndex: 1
    property string leftRecordLabel: "UNLABELED"
    property string rightRecordLabel: "UNLABELED"
    readonly property var liveDiagnostics: uiState.gestureDiagnostics
        && uiState.gestureDiagnostics.length > liveHandIndex
        ? uiState.gestureDiagnostics[liveHandIndex] : ({})
    readonly property bool liveHandIsLeft: liveHandIndex === 0
    readonly property bool liveHandTracked: liveHandIsLeft
        ? uiState.leftTracked
        : uiState.rightTracked
    readonly property string liveGesture: liveHandIsLeft
        ? uiState.leftGesture
        : uiState.rightGesture
    readonly property real liveConfidence: liveHandIsLeft
        ? uiState.leftGestureConfidence
        : uiState.rightGestureConfidence
    readonly property string liveEvent: liveHandIsLeft
        ? uiState.leftGestureEvent
        : uiState.rightGestureEvent

    function localizedFrameCount(count) {
        // Avoid the invalid '1 klatek' / '2 klatek' forms in Polish.
        const n = Math.max(0, Math.floor(Number(count) || 0))
        if (LanguageManager.language === "pl") {
            const few = n % 10 >= 2 && n % 10 <= 4
                && (n % 100 < 12 || n % 100 > 14)
            return n + (n === 1 ? " klatka" : few ? " klatki" : " klatek")
        }
        return n + (n === 1 ? " frame" : " frames")
    }

    function localizedLabel(value) {
        LanguageManager.language
        return qsTr(value)
    }

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
            title: qsTr("Gestures")
            subtitle: qsTr("M5 gesture reliability, classifier diagnostics and labelled landmark capture.")

            VcTextField {
                theme: root.theme
                placeholderText: qsTr("Search gestures (planned)")
                implicitWidth: 210
                enabled: false
            }

            VcButton {
                theme: root.theme
                text: qsTr("New gesture")
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
                            text: qsTr("Gesture Library")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        Repeater {
                            model: [qsTr("All"), qsTr("Static"), qsTr("Dynamic"), qsTr("Two-hand")]

                            delegate: Rectangle {
                                required property string modelData

                                width: chipText.implicitWidth + 18
                                height: 30
                                color: modelData === qsTr("All")
                                    ? root.theme.surfaceHover
                                    : "transparent"
                                border.width: 1
                                border.color: modelData === qsTr("All")
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
                                                    text: root.localizedLabel(modelData.category)
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
                                                        ? qsTr("Available")
                                                        : qsTr("Planned")
                                                    kind: modelData.enabled
                                                        ? "success"
                                                        : "neutral"
                                                }

                                                Item {
                                                    Layout.fillWidth: true
                                                }

                                                Text {
                                                    text: qsTr("View details  ›")
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
                Layout.preferredWidth: 360
                Layout.minimumWidth: 340
                Layout.fillHeight: true
                theme: root.theme

                ScrollView {
                    id: liveScroll
                    anchors.fill: parent
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: liveScroll.availableWidth
                        spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "camera"
                        title: qsTr("Live Recognition")
                        statusText: !root.uiState.gestureRecognitionAvailable
                            ? qsTr("Unavailable")
                            : (!root.uiState.pipelineRunning
                                ? qsTr("Off")
                                : root.liveHandTracked ? qsTr("Live") : qsTr("Hand lost"))
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
                        color: root.theme.surfaceRaised
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
                                ? qsTr("GESTURE RECOGNITION UNAVAILABLE")
                                : (root.uiState.pipelineRunning
                                    ? qsTr("HAND NOT TRACKED")
                                    : qsTr("PIPELINE STOPPED"))
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
                            model: [qsTr("Left hand"), qsTr("Right hand")]
                            currentIndex: root.liveHandIndex
                            implicitWidth: 132
                            onActivated: root.liveHandIndex = currentIndex
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 10
                            anchors.bottomMargin: 10
                            text: root.liveHandIsLeft ? qsTr("LEFT HAND") : qsTr("RIGHT HAND")
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
                            text: qsTr("Detected gesture")
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }

                        Text {
                            text: root.uiState.gestureRecognitionAvailable
                                ? root.localizedLabel(root.liveGesture)
                                : qsTr("Unavailable")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Gesture confidence")
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

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Event state")
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                            }
                            StatusBadge {
                                theme: root.theme
                                text: root.localizedLabel(root.liveEvent)
                                kind: root.liveEvent === "PRESS"
                                    ? "success"
                                    : root.liveEvent === "RELEASE"
                                        ? "warning"
                                        : "neutral"
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Backend: ") + root.uiState.gestureBackendName
                                + "\n" + root.uiState.gestureModelStatus
                                + qsTr("\nRules scores are heuristics, not calibrated probabilities.")
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.theme.borderSubtle
                        }

                        Text {
                            text: qsTr("Classifier diagnostics")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Reason: ") + (root.liveDiagnostics.reason || "--")
                                + qsTr(" | active: ") + (root.liveDiagnostics.active || "NONE")
                                + qsTr(" | candidate: ") + (root.liveDiagnostics.candidate || "NONE")
                                + (root.liveDiagnostics.inGrace ? qsTr(" | GRACE") : "")
                                + (root.liveDiagnostics.inCooldown ? qsTr(" | COOLDOWN") : "")
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }
                        Repeater {
                            model: root.liveDiagnostics.scores || []
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 6
                                Text {
                                    Layout.preferredWidth: 86
                                    text: modelData.name
                                    color: root.theme.textMuted
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 9
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 5
                                    color: root.theme.borderStrong
                                    Rectangle {
                                        width: parent.width * Math.max(0, Math.min(1, modelData.value))
                                        height: parent.height
                                        color: root.theme.accent
                                    }
                                }
                                Text {
                                    Layout.preferredWidth: 35
                                    horizontalAlignment: Text.AlignRight
                                    text: Math.round(modelData.value * 100) + "%"
                                    color: root.theme.textSecondary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 9
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Curl [I/M/R/L]: ") + (root.liveDiagnostics.curl || []).map(
                                function(v) { return Number(v).toFixed(2) }).join(" / ")
                                + qsTr("\nPinch distance: ") + Number(root.liveDiagnostics.pinch || 0).toFixed(3)
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.theme.borderSubtle
                        }
                        Text {
                            text: qsTr("Labelled landmark recording")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Set the actual gesture for each hand before recording. No camera images are saved.")
                            wrapMode: Text.WordWrap
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            VcComboBox {
                                Layout.fillWidth: true
                                theme: root.theme
                                model: ["UNLABELED", "NONE", "FIST", "OPEN_HAND", "POINT", "PINCH"]
                                enabled: !root.uiState.recordingActive
                                onActivated: root.leftRecordLabel = currentText
                            }
                            VcComboBox {
                                Layout.fillWidth: true
                                theme: root.theme
                                model: ["UNLABELED", "NONE", "FIST", "OPEN_HAND", "POINT", "PINCH"]
                                enabled: !root.uiState.recordingActive
                                onActivated: root.rightRecordLabel = currentText
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("LEFT label")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 9
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("RIGHT label")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 9
                            }
                        }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: root.uiState.recordingActive ? qsTr("Stop recording") : qsTr("Record samples")
                            iconName: "camera"
                            enabled: root.uiState.recordingActive || root.uiState.pipelineRunning
                            onClicked: {
                                if (root.uiState.recordingActive)
                                    root.uiState.stopGestureRecording()
                                else
                                    root.uiState.startGestureRecording(root.leftRecordLabel, root.rightRecordLabel)
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: root.uiState.recordingActive
                                ? qsTr("Recording: ") + root.localizedFrameCount(root.uiState.recordedFrames)
                                : qsTr("Last session: ") + root.localizedFrameCount(root.uiState.recordedFrames)
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: root.uiState.recordingPath !== ""
                            text: root.uiState.recordingPath
                            wrapMode: Text.WrapAnywhere
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: root.uiState.recordingError !== ""
                            text: root.uiState.recordingError
                            wrapMode: Text.WordWrap
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        Item { Layout.preferredHeight: 16 }
                    }
                    }
                }
            }
        }
    }
}