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
                            onFullscreenRequested: root.openDashboardRequested()
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 9
                            Repeater {
                                model: [
                                    { name: qsTr("Left hand"), tracked: root.uiState.leftTracked,
                                      gesture: root.uiState.leftGesture, color: root.theme.leftHand },
                                    { name: qsTr("Right hand"), tracked: root.uiState.rightTracked,
                                      gesture: root.uiState.rightGesture, color: root.theme.rightHand }
                                ]
                                delegate: Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    Layout.preferredHeight: 57
                                    radius: root.theme.controlRadius
                                    color: root.theme.surfaceRaised
                                    border.color: root.theme.borderSubtle
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 9
                                        spacing: 3
                                        Text {
                                            text: modelData.name
                                            color: root.theme.textMuted
                                            font.pixelSize: 10
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.tracked ? root.localizedGesture(modelData.gesture) : qsTr("Not detected")
                                            color: modelData.tracked ? modelData.color : root.theme.textSecondary
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
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
