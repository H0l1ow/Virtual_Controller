import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VirtualController.I18n 1.0
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property bool previewActive: false
    // One section at a time, one outer scroll area. No nested scrollbars.
    property int activeSection: 0
    signal openDashboardRequested()
    readonly property var gestureItems: [
        { code: "OPEN_HAND", title: qsTr("Open Hand"), icon: "open",
            hint: qsTr("All four fingers extended") },
        { code: "POINT", title: qsTr("Point"), icon: "point",
            hint: qsTr("Index finger extended") },
        { code: "FIST", title: qsTr("Fist"), icon: "fist",
            hint: qsTr("Four fingers curled") },
        { code: "PINCH", title: qsTr("Pinch"), icon: "pinch",
            hint: qsTr("Thumb and index finger close") }
    ]
    function localizedGesture(value) {
        LanguageManager.language
        return qsTr(value)
    }
    function experimentalRanking(diagnostic) {
        // Only compare rule scores within the experimental channel. Mixing
        // five-class ONNX confidences and rule scores is not meaningful.
        if (!diagnostic || !diagnostic.scores)
            return []
        return diagnostic.scores.slice(4).sort((a, b) => b.value - a.value).slice(0, 3)
    }
    function diagnosticReason(code) {
        LanguageManager.language
        switch (code) {
        case "AMBIGUOUS": return qsTr("Ambiguous pose")
        case "POSE_STABILIZING": return qsTr("Stabilizing pose")
        case "RECOGNIZED": return qsTr("Recognized")
        case "BELOW_THRESHOLD": return qsTr("Below threshold")
        case "NONE_DOMINANT": return qsTr("Neutral pose")
        case "MODEL_WARMUP": return qsTr("Model warmup")
        case "INVALID_GEOMETRY": return qsTr("Invalid geometry")
        default: return qsTr("No tracking")
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        ColumnLayout {
            width: Math.max(1, scroll.availableWidth)
            spacing: 13
            Item { Layout.preferredHeight: 3 }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: qsTr("Gestures")
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: root.theme.fontHero
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Live recognition, class thresholds and calibration")
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: root.theme.fontBody
                        elide: Text.ElideRight
                    }
                }
                Text {
                    visible: root.uiState.hasUnsavedSettings
                    text: qsTr("Unsaved changes")
                    color: root.theme.warning
                    font.pixelSize: 11
                }
                VcButton {
                    theme: root.theme
                    text: qsTr("Save")
                    iconName: "save"
                    enabled: root.uiState.hasUnsavedSettings
                    onClicked: root.uiState.saveSettings()
                }
            }
            GridLayout {
                id: gestureSections
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                columns: root.width >= 980 ? 3 : (root.width >= 650 ? 2 : 1)
                columnSpacing: 8
                rowSpacing: 8
                Repeater {
                    model: [qsTr("Gestures"), qsTr("Hand calibration"), qsTr("Advanced thresholds & timing")]
                    delegate: VcButton {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        theme: root.theme
                        text: modelData
                        variant: root.activeSection === index ? "primary" : "secondary"
                        onClicked: root.activeSection = index
                    }
                }
            }
            GridLayout {
                visible: root.activeSection === 0
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                columns: root.width >= 1170 ? 2 : 1
                columnSpacing: 13
                rowSpacing: 13
                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: root.width < 590 ? 730 : 425
                    theme: root.theme
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        Text {
                            text: qsTr("Gesture thresholds")
                            color: root.theme.textPrimary
                            font.pixelSize: 16
                            font.family: root.theme.fontFamily
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Adjust the required score for each recognized gesture. Changes apply to both hands.")
                            wrapMode: Text.WordWrap
                            color: root.theme.textMuted
                            font.pixelSize: 11
                        }
                        GridLayout {
                            id: thresholdGrid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: width >= 520 ? 2 : 1
                            rowSpacing: 9
                            columnSpacing: 10
                            Repeater {
                                model: root.gestureItems
                                delegate: Rectangle {
                                    id: gestureCard
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 0
                                    Layout.minimumHeight: 140
                                    radius: root.theme.controlRadius
                                    color: root.theme.surfaceRaised
                                    border.width: 1
                                    border.color: root.theme.border
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 11
                                        spacing: 7
                                        RowLayout {
                                            Layout.fillWidth: true
                                            GestureGlyph {
                                                gesture: gestureCard.modelData.icon
                                                Layout.preferredWidth: 32
                                                Layout.preferredHeight: 32
                                                color: root.theme.accent
                                            }
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 2
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: gestureCard.modelData.title
                                                    color: root.theme.textPrimary
                                                    font.family: root.theme.fontFamily
                                                    font.pixelSize: 13
                                                    font.weight: Font.DemiBold
                                                    elide: Text.ElideRight
                                                }
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: gestureCard.modelData.hint
                                                    color: root.theme.textMuted
                                                    font.family: root.theme.fontFamily
                                                    font.pixelSize: 10
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }
                                        Item { Layout.fillHeight: true }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                Layout.fillWidth: true
                                                text: qsTr("Threshold")
                                                color: root.theme.textSecondary
                                                font.pixelSize: 11
                                            }
                                            Text {
                                                text: Number(root.uiState.gestureThresholds[gestureCard.modelData.code]) + "%"
                                                color: root.theme.accent
                                                font.pixelSize: 12
                                                font.weight: Font.DemiBold
                                            }
                                            VcIconButton {
                                                theme: root.theme
                                                iconName: "reset"
                                                implicitHeight: 29
                                                implicitWidth: 29
                                                enabled: root.uiState.gestureThresholdOverrides[gestureCard.modelData.code] >= 0
                                                onClicked: root.uiState.setGestureThreshold(gestureCard.modelData.code, -1)
                                            }
                                        }
                                        VcSlider {
                                            theme: root.theme
                                            Layout.fillWidth: true
                                            from: 60
                                            to: 100
                                            stepSize: 1
                                            value: Number(root.uiState.gestureThresholds[gestureCard.modelData.code])
                                            onMoved: root.uiState.setGestureThreshold(gestureCard.modelData.code, value)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: 425
                    theme: root.theme
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 9
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Live recognition")
                                font.family: root.theme.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                color: root.theme.textPrimary
                            }
                            StatusBadge {
                                theme: root.theme
                                text: root.uiState.pipelineRunning ? qsTr("Live") : qsTr("Off")
                                kind: root.uiState.pipelineRunning ? "success" : "neutral"
                            }
                        }
                        CameraStage {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 220
                            theme: root.theme
                            uiState: root.uiState
                            previewActive: root.previewActive
                            immersiveMode: false
                            showGestureHints: false
                            showHandBadges: false
                            onFullscreenRequested: root.openDashboardRequested()
                        }
                    }
                }
            }
            VcCard {
                visible: root.activeSection === 0
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                Layout.preferredHeight: root.width >= 750 ? 265 : 475
                theme: root.theme
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Recognition diagnostics")
                            font.family: root.theme.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            color: root.theme.textPrimary
                        }
                        StatusBadge {
                            theme: root.theme
                            text: root.uiState.pipelineRunning ? qsTr("Live") : qsTr("Off")
                            kind: root.uiState.pipelineRunning ? "success" : "neutral"
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Experimental scores are heuristic, not probabilities. A small gap between the first two poses suppresses input.")
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: root.width >= 750 ? 2 : 1
                        rowSpacing: 8
                        columnSpacing: 10
                        Repeater {
                            model: [0, 1]
                            delegate: Rectangle {
                                id: diagnosticHand
                                required property int modelData
                                readonly property var diagnostic:
                                    (root.uiState.gestureDiagnostics || [])[modelData] || ({})
                                readonly property var ranked: root.experimentalRanking(diagnostic)
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                Layout.fillHeight: true
                                radius: root.theme.controlRadius
                                color: root.theme.surfaceRaised
                                border.width: 1
                                border.color: root.theme.borderSubtle
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 4
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            Layout.fillWidth: true
                                            text: diagnosticHand.modelData === 0
                                                  ? qsTr("Left hand") : qsTr("Right hand")
                                            color: root.theme.textPrimary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                        }
                                        Text {
                                            text: root.diagnosticReason(diagnosticHand.diagnostic.reason)
                                            color: (diagnosticHand.diagnostic.reason === "AMBIGUOUS"
                                                || diagnosticHand.diagnostic.reason === "POSE_STABILIZING")
                                                ? root.theme.warning : root.theme.textSecondary
                                            font.pixelSize: 10
                                        }
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("Recognized") + ": "
                                              + (diagnosticHand.diagnostic.raw || "NONE")
                                        color: root.theme.textSecondary
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("Experimental gap") + ": "
                                              + (100 * Number(diagnosticHand.diagnostic.experimentalMargin || 0)).toFixed(0)
                                              + " pp · " + qsTr("stable") + ": "
                                              + Number(diagnosticHand.diagnostic.experimentalStableMs || 0) + " ms"
                                        color: root.theme.textMuted
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                    Repeater {
                                        model: diagnosticHand.ranked
                                        delegate: RowLayout {
                                            id: scoreRow
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 6
                                            Text {
                                                text: scoreRow.modelData.name.split("_").join(" ")
                                                Layout.preferredWidth: 102
                                                color: root.theme.textSecondary
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                            }
                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 7
                                                radius: 3
                                                color: root.theme.borderSubtle
                                                Rectangle {
                                                    width: parent.width * Math.max(0, Math.min(1, Number(scoreRow.modelData.value)))
                                                    height: parent.height
                                                    radius: parent.radius
                                                    color: root.theme.accent
                                                }
                                            }
                                            Text {
                                                Layout.preferredWidth: 33
                                                horizontalAlignment: Text.AlignRight
                                                text: Math.round(100 * Number(scoreRow.modelData.value)) + "%"
                                                color: root.theme.textPrimary
                                                font.pixelSize: 10
                                            }
                                        }
                                    }
                                    Item { Layout.fillHeight: true }
                                }
                            }
                        }
                    }
                }
            }
            VcCard {
                visible: root.activeSection === 0
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                Layout.preferredHeight: root.width >= 750 ? 590 : (root.width >= 490 ? 770 : 1280)
                theme: root.theme
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    Text {
                        text: qsTr("Gesture library")
                        font.family: root.theme.fontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: root.theme.textPrimary
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Only available gestures can be mapped. Planned gestures require implementation or training.")
                        font.pixelSize: 11
                        color: root.theme.textMuted
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Experimental recognition uses landmark rules; test reliability before assigning system input.")
                        color: root.theme.warning
                        font.pixelSize: 11
                    }
                    Repeater {
                        model: [
                            {category: "static", title: qsTr("Static gestures")},
                            {category: "dynamic", title: qsTr("Dynamic gestures")},
                            {category: "temporal", title: qsTr("Temporal gestures")}
                        ]
                        delegate: ColumnLayout {
                            id: gestureGroup
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                text: gestureGroup.modelData.title
                                color: root.theme.textSecondary
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.width >= 750 ? 3 : (root.width >= 490 ? 2 : 1)
                                columnSpacing: 7
                                rowSpacing: 6
                                Repeater {
                                    model: (root.uiState.gestureCatalog || []).filter(
                                        entry => entry.category === gestureGroup.modelData.category)
                                    delegate: Rectangle {
                                        id: catalogGesture
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        Layout.preferredHeight: 35
                                        color: root.theme.surfaceRaised
                                        border.width: 1
                                        border.color: root.theme.borderSubtle
                                        radius: root.theme.smallRadius
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 9
                                            anchors.rightMargin: 8
                                            spacing: 6
                                            Text {
                                                Layout.fillWidth: true
                                                text: root.localizedGesture(catalogGesture.modelData.title)
                                                color: catalogGesture.modelData.available
                                                    ? root.theme.textPrimary : root.theme.textMuted
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }
                                            StatusBadge {
                                                theme: root.theme
                                                text: catalogGesture.modelData.experimental
                                                    ? qsTr("Experimental")
                                                    : (catalogGesture.modelData.available
                                                        ? qsTr("Available") : qsTr("Planned"))
                                                kind: catalogGesture.modelData.experimental ? "warning"
                                                    : (catalogGesture.modelData.available ? "success" : "neutral")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
            // These forms share the PAGE scroll area; their contents no longer
            // own ScrollViews. Switching categories never creates a scrollbar inside
            // a second scrollbar.
            CalibrationPanel {
                id: calibrationPanel
                visible: root.activeSection === 1
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                Layout.preferredHeight: implicitHeight
                theme: root.theme
                uiState: root.uiState
            }
            GestureTuningPanel {
                id: timingPanel
                visible: root.activeSection === 2
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                Layout.preferredHeight: implicitHeight
                theme: root.theme
                uiState: root.uiState
            }
            Item { Layout.preferredHeight: root.theme.pageMargin }
        }
    }
}
