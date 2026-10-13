import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property string selectedHand: "Right"
    property string selectedGesture: "PINCH"
    readonly property var classes: ["FIST", "OPEN_HAND", "POINT", "PINCH"]
    readonly property var sides: ["Left", "Right"]

    function labelForGesture(code) {
        switch (code) {
        case "FIST": return qsTr("Fist")
        case "OPEN_HAND": return qsTr("Open Hand")
        case "POINT": return qsTr("Point")
        case "PINCH": return qsTr("Pinch")
        }
        return code
    }
    function labelForSide(code) {
        return code === "Left" ? qsTr("Left") : qsTr("Right")
    }
    function statusText(code) {
        switch (code) {
        case "CAPTURING": return qsTr("Keep the selected pose steady until sampling finishes.")
        case "READY": return qsTr("Calibration ready. Review the suggested threshold and apply it.")
        case "WEAK_SIGNAL": return qsTr("Gesture score too weak. Adjust your pose and retry.")
        case "AMBIGUOUS": return qsTr("Gesture overlaps another class. Change the pose and retry.")
        case "CANCELLED": return qsTr("Calibration cancelled.")
        default: return qsTr("Select a hand and a gesture, then capture 36 valid frames.")
        }
    }
    function suggestedThreshold(code) {
        const results = root.uiState.calibrationResults || []
        for (let i = 0; i < results.length; i++) {
            if (results[i].gesture === code && results[i].suggested >= 0)
                return results[i].suggested
        }
        return -1
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentHeight: contents.implicitHeight + 36

        ColumnLayout {
            id: contents
            x: 18
            y: 18
            width: Math.max(1, scroll.availableWidth - 36)
            spacing: 13

            Text {
                Layout.fillWidth: true
                text: qsTr("Hand calibration")
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Capture each pose with your left and right hand. This calibrates the recognition thresholds, not the MediaPipe landmark model.")
                wrapMode: Text.WordWrap
                color: root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
            SettingsRow {
                Layout.fillWidth: true
                theme: root.theme
                title: qsTr("Hand")
                description: qsTr("Both hands are sampled separately.")
                VcComboBox {
                    theme: root.theme
                    model: root.sides
                    labelFormatter: root.labelForSide
                    currentIndex: root.sides.indexOf(root.selectedHand)
                    implicitWidth: 175
                    enabled: !root.uiState.calibrationActive
                    onActivated: root.selectedHand = root.sides[currentIndex]
                }
            }
            SettingsRow {
                Layout.fillWidth: true
                theme: root.theme
                title: qsTr("Gesture")
                description: qsTr("Choose the pose you will hold in front of the camera.")
                VcComboBox {
                    theme: root.theme
                    model: root.classes
                    labelFormatter: root.labelForGesture
                    currentIndex: root.classes.indexOf(root.selectedGesture)
                    implicitWidth: 175
                    enabled: !root.uiState.calibrationActive
                    onActivated: root.selectedGesture = root.classes[currentIndex]
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                VcButton {
                    theme: root.theme
                    text: qsTr("Capture samples")
                    iconName: "camera"
                    enabled: root.uiState.pipelineRunning && !root.uiState.calibrationActive
                    onClicked: root.uiState.startGestureCalibration(root.selectedHand, root.selectedGesture)
                }
                VcButton {
                    theme: root.theme
                    text: qsTr("Cancel")
                    iconName: "reset"
                    enabled: root.uiState.calibrationActive
                    onClicked: root.uiState.cancelGestureCalibration()
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.uiState.calibrationProgress + "%"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 12
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 9
                radius: 5
                color: root.theme.borderStrong
                Rectangle {
                    height: parent.height
                    width: Math.max(0, parent.width * root.uiState.calibrationProgress / 100)
                    radius: parent.radius
                    color: root.theme.accent
                }
            }
            Text {
                Layout.fillWidth: true
                text: root.statusText(root.uiState.calibrationStatus)
                wrapMode: Text.WordWrap
                color: root.uiState.calibrationStatus === "WEAK_SIGNAL"
                    || root.uiState.calibrationStatus === "AMBIGUOUS"
                    ? root.theme.warning : root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Calibration disables Windows input. Re-arm input explicitly after testing. Samples are temporary until you apply a suggestion and Save settings.")
                wrapMode: Text.WordWrap
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
            Text {
                text: qsTr("Recommended thresholds")
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Recommendations are heuristic, not measured recognition accuracy. For classes captured with both hands, the less confident hand determines the suggestion.")
                wrapMode: Text.WordWrap
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 10
            }
            Repeater {
                model: root.classes
                delegate: RowLayout {
                    id: recommendationRow
                    required property string modelData
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        text: root.labelForGesture(recommendationRow.modelData)
                        font.family: root.theme.fontFamily
                        font.pixelSize: 12
                        color: root.theme.textPrimary
                    }
                    Text {
                        text: root.suggestedThreshold(recommendationRow.modelData) < 0
                            ? qsTr("Not sampled") : root.suggestedThreshold(recommendationRow.modelData) + "%"
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                        color: root.theme.textSecondary
                    }
                    VcButton {
                        theme: root.theme
                        text: qsTr("Apply")
                        iconName: "save"
                        enabled: root.suggestedThreshold(recommendationRow.modelData) >= 0
                            && !root.uiState.calibrationActive
                        onClicked: root.uiState.applyGestureCalibration(recommendationRow.modelData)
                    }
                }
            }
            VcButton {
                Layout.alignment: Qt.AlignLeft
                theme: root.theme
                text: qsTr("Clear calibration samples")
                iconName: "reset"
                enabled: !root.uiState.calibrationActive
                onClicked: root.uiState.clearGestureCalibration()
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}
