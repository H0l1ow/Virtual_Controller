import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    enabled: root.uiState.gestureRecognitionAvailable

    implicitHeight: gesturesSettingsColumn.implicitHeight + 36
        ColumnLayout {
            id: gesturesSettingsColumn
            x: 18
            y: 18
            width: Math.max(1, root.width - 36)
            spacing: 0
        Text {
            text: qsTr("Gestures")
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }
        Text {
            text: qsTr("Gesture event thresholds and state behavior.")
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: 10
            Layout.bottomMargin: 16
        }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Recognition threshold")
            description: qsTr("Minimum recognition confidence before a gesture is reported as active.")
            VcSlider {
                id: recognitionThresholdSlider
                theme: root.theme
                value: root.uiState.recognitionThreshold
                implicitWidth: 230
                onMoved: root.uiState.setRecognitionThreshold(value)
            }
            Text {
                text: Math.round(recognitionThresholdSlider.value) + "%"
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }
        Text {
            Layout.topMargin: 18
            Layout.bottomMargin: 7
            text: qsTr("Individual gesture thresholds")
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }
        Text {
            Layout.fillWidth: true
            Layout.bottomMargin: 10
            text: qsTr("Each class inherits the global threshold unless customized. Changes affect both hands and are saved with application settings, not mapping profiles.")
            wrapMode: Text.WordWrap
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pixelSize: 10
        }
        Repeater {
            model: [
                {code: "FIST", label: qsTr("Fist")},
                {code: "OPEN_HAND", label: qsTr("Open Hand")},
                {code: "POINT", label: qsTr("Point")},
                {code: "PINCH", label: qsTr("Pinch")}
            ]
            delegate: SettingsRow {
                id: thresholdSettingRow
                required property var modelData
                Layout.fillWidth: true
                theme: root.theme
                title: thresholdSettingRow.modelData.label
                description: root.uiState.gestureThresholdOverrides[thresholdSettingRow.modelData.code] < 0
                    ? qsTr("Using global threshold")
                    : qsTr("Custom recognition threshold")
                VcSlider {
                    id: classSlider
                    theme: root.theme
                    implicitWidth: 145
                    from: 60
                    to: 100
                    stepSize: 1
                    value: Number(root.uiState.gestureThresholds[thresholdSettingRow.modelData.code])
                    onMoved: root.uiState.setGestureThreshold(thresholdSettingRow.modelData.code, value)
                }
                Text {
                    text: Math.round(classSlider.value) + "%"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                }
                VcIconButton {
                    theme: root.theme
                    iconName: "reset"
                    implicitWidth: 34
                    implicitHeight: 34
                    enabled: root.uiState.gestureThresholdOverrides[thresholdSettingRow.modelData.code] >= 0
                    onClicked: root.uiState.setGestureThreshold(thresholdSettingRow.modelData.code, -1)
                }
            }
        }
        VcButton {
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            Layout.alignment: Qt.AlignLeft
            theme: root.theme
            text: qsTr("Reset custom thresholds")
            iconName: "reset"
            onClicked: root.uiState.resetGestureThresholds()
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Release threshold")
            description: qsTr("Lower threshold for keeping a held gesture during a brief low-confidence frame.")
            VcComboBox {
                id: releaseThresholdCombo
                theme: root.theme
                model: ["40 %", "50 %", "55 %", "60 %", "70 %"]
                currentIndex: Math.max(0, [40, 50, 55, 60, 70].indexOf(root.uiState.gestureReleaseThreshold))
                implicitWidth: 180
                onActivated: root.uiState.setGestureReleaseThreshold(parseInt(currentText))
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Tracking jitter grace")
            description: qsTr("Ignore short misclassifications without releasing a held input. Real hand loss still releases immediately.")
            VcComboBox {
                id: gestureGraceCombo
                theme: root.theme
                model: ["0 ms", "50 ms", "100 ms", "150 ms", "200 ms"]
                currentIndex: Math.max(0, [0, 50, 100, 150, 200].indexOf(root.uiState.gestureGraceMs))
                implicitWidth: 180
                onActivated: root.uiState.setGestureGraceMs(parseInt(currentText))
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Debounce")
            description: qsTr("Gesture must remain stable for this long before PRESS is emitted.")
            VcComboBox {
                id: debounceCombo
                theme: root.theme
                model: ["80 ms", "120 ms", "160 ms", "200 ms"]
                currentIndex: Math.max(0, [80, 120, 160, 200].indexOf(root.uiState.debounceMs))
                implicitWidth: 180
                onActivated: root.uiState.setDebounceMs(parseInt(currentText))
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Cooldown")
            description: qsTr("Minimum quiet time after RELEASE before another PRESS can fire.")
            VcComboBox {
                id: cooldownCombo
                theme: root.theme
                model: ["150 ms", "250 ms", "400 ms", "600 ms"]
                currentIndex: Math.max(0, [150, 250, 400, 600].indexOf(root.uiState.cooldownMs))
                implicitWidth: 180
                onActivated: root.uiState.setCooldownMs(parseInt(currentText))
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.theme.borderSubtle
        }
        SettingsRow {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Require release")
            description: qsTr("Require one complete observation away from the released gesture before it can trigger again.")
            VcSwitch {
                id: requireReleaseSwitch
                theme: root.theme
                checked: root.uiState.requireRelease
                onToggled: root.uiState.setRequireRelease(checked)
            }
        }
        Item { Layout.preferredHeight: 12 }
        }


    Connections {
        target: root.uiState
        function onRecognitionThresholdChanged() {
            if (!recognitionThresholdSlider.pressed)
                recognitionThresholdSlider.value = root.uiState.recognitionThreshold
        }
        function onRequireReleaseChanged() {
            if (requireReleaseSwitch.checked !== root.uiState.requireRelease)
                requireReleaseSwitch.checked = root.uiState.requireRelease
        }
        function onDebounceMsChanged() {
            const index = [80, 120, 160, 200].indexOf(root.uiState.debounceMs)
            if (index >= 0 && debounceCombo.currentIndex !== index)
                debounceCombo.currentIndex = index
        }
        function onCooldownMsChanged() {
            const index = [150, 250, 400, 600].indexOf(root.uiState.cooldownMs)
            if (index >= 0 && cooldownCombo.currentIndex !== index)
                cooldownCombo.currentIndex = index
        }
    }
}
