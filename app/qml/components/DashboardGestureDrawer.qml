import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Compact, functional gesture controls for the dashboard. The full gesture
// page retains range calibration and advanced timing parameters.
Drawer {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int section: 0
    property string selectedHand: "Right"
    property string selectedGesture: "PINCH"
    readonly property var gestureClasses: ["OPEN_HAND", "POINT", "FIST", "PINCH"]
    readonly property int selectedSuggestedThreshold: {
        const results = root.uiState.calibrationResults || []
        for (const result of results) {
            if (result.gesture === root.selectedGesture && result.suggested >= 0)
                return result.suggested
        }
        return -1
    }

    signal fullSettingsRequested()

    function gestureLabel(value) {
        switch (value) {
        case "OPEN_HAND": return qsTr("Open Hand")
        case "POINT": return qsTr("Point")
        case "FIST": return qsTr("Fist")
        case "PINCH": return qsTr("Pinch")
        default: return value
        }
    }
    function handLabel(value) {
        return value === "Left" ? qsTr("Left") : qsTr("Right")
    }
    function calibrationMessage(status) {
        switch (status) {
        case "CAPTURING": return qsTr("Hold the selected gesture steady")
        case "READY": return qsTr("Samples captured. Apply the suggested threshold.")
        case "WEAK_SIGNAL": return qsTr("Weak signal. Change the pose and retry.")
        case "AMBIGUOUS": return qsTr("Ambiguous pose. Change the gesture and retry.")
        case "CANCELLED": return qsTr("Calibration cancelled")
        default: return qsTr("Choose a hand and gesture to calibrate")
        }
    }

    edge: Qt.RightEdge
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        color: root.theme.backgroundAlt
        border.width: 1
        border.color: root.theme.borderStrong
    }
    contentItem: Loader {
        active: root.visible
        sourceComponent: Component {
            ScrollView {
                id: drawerScroll
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    x: 14
                    y: 14
                    width: Math.max(1, drawerScroll.availableWidth - 28)
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Gestures & calibration")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        VcIconButton {
                            theme: root.theme
                            iconName: "stop"
                            onClicked: root.close()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: qsTr("Gestures")
                            variant: root.section === 0 ? "primary" : "secondary"
                            onClicked: root.section = 0
                        }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: qsTr("Hand calibration")
                            variant: root.section === 1 ? "primary" : "secondary"
                            onClicked: root.section = 1
                        }
                    }

                    ColumnLayout {
                        visible: root.section === 0
                        Layout.fillWidth: true
                        spacing: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Gesture thresholds")
                            color: root.theme.textPrimary
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        Repeater {
                            model: root.gestureClasses
                            delegate: ColumnLayout {
                                id: thresholdRow
                                required property string modelData
                                Layout.fillWidth: true
                                spacing: 3
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.gestureLabel(thresholdRow.modelData)
                                        color: root.theme.textSecondary
                                        font.pixelSize: 12
                                    }
                                    Text {
                                        text: Number(root.uiState.gestureThresholds[thresholdRow.modelData]) + "%"
                                        color: root.theme.textPrimary
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }
                                    VcIconButton {
                                        theme: root.theme
                                        iconName: "reset"
                                        implicitWidth: 30
                                        implicitHeight: 30
                                        enabled: root.uiState.gestureThresholdOverrides[thresholdRow.modelData] >= 0
                                        onClicked: root.uiState.setGestureThreshold(thresholdRow.modelData, -1)
                                    }
                                }
                                VcSlider {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    enabled: root.uiState.gestureRecognitionAvailable
                                    from: 60
                                    to: 100
                                    stepSize: 1
                                    value: Number(root.uiState.gestureThresholds[thresholdRow.modelData])
                                    onMoved: root.uiState.setGestureThreshold(thresholdRow.modelData, value)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        visible: root.section === 1
                        Layout.fillWidth: true
                        spacing: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Hand calibration")
                            color: root.theme.textPrimary
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Hand")
                                color: root.theme.textSecondary
                                font.pixelSize: 12
                            }
                            VcComboBox {
                                theme: root.theme
                                implicitWidth: 174
                                model: ["Left", "Right"]
                                labelFormatter: root.handLabel
                                currentIndex: root.selectedHand === "Left" ? 0 : 1
                                enabled: !root.uiState.calibrationActive && !root.uiState.rangeCalibrationActive
                                onActivated: root.selectedHand = currentText
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Gesture")
                                color: root.theme.textSecondary
                                font.pixelSize: 12
                            }
                            VcComboBox {
                                theme: root.theme
                                implicitWidth: 174
                                model: root.gestureClasses
                                labelFormatter: root.gestureLabel
                                currentIndex: root.gestureClasses.indexOf(root.selectedGesture)
                                enabled: !root.uiState.calibrationActive && !root.uiState.rangeCalibrationActive
                                onActivated: root.selectedGesture = currentText
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            VcButton {
                                Layout.fillWidth: true
                                theme: root.theme
                                text: qsTr("Capture samples")
                                iconName: "camera"
                                enabled: root.uiState.pipelineRunning
                                    && root.uiState.gestureRecognitionAvailable
                                    && !root.uiState.calibrationActive
                                    && !root.uiState.rangeCalibrationActive
                                onClicked: root.uiState.startGestureCalibration(root.selectedHand, root.selectedGesture)
                            }
                            VcButton {
                                theme: root.theme
                                text: qsTr("Cancel")
                                enabled: root.uiState.calibrationActive
                                onClicked: root.uiState.cancelGestureCalibration()
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 8
                            radius: 4
                            color: root.theme.borderStrong
                            Rectangle {
                                height: parent.height
                                width: parent.width * Math.max(0, Math.min(100, root.uiState.calibrationProgress)) / 100
                                radius: parent.radius
                                color: root.theme.accent
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: root.calibrationMessage(root.uiState.calibrationStatus)
                                + "  (" + root.uiState.calibrationProgress + "%)"
                            wrapMode: Text.WordWrap
                            color: root.theme.textSecondary
                            font.pixelSize: 12
                        }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: root.selectedSuggestedThreshold >= 0
                                ? qsTr("Apply suggestion") + " (" + root.selectedSuggestedThreshold + "%)"
                                : qsTr("Apply suggestion")
                            iconName: "save"
                            enabled: root.selectedSuggestedThreshold >= 0
                                && !root.uiState.calibrationActive
                            onClicked: root.uiState.applyGestureCalibration(root.selectedGesture)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Calibration pauses system input. After calibration, enable it again if needed.")
                            wrapMode: Text.WordWrap
                            color: root.theme.textMuted
                            font.pixelSize: 11
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.borderSubtle
                    }
                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: qsTr("Save settings")
                        iconName: "save"
                        variant: "primary"
                        enabled: root.uiState.hasUnsavedSettings
                        onClicked: root.uiState.saveSettings()
                    }
                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: qsTr("All gesture settings")
                        variant: "ghost"
                        onClicked: {
                            root.close()
                            root.fullSettingsRequested()
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }
        }
    }
}
