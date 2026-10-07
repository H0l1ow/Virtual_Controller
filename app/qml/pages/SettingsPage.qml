import QtQuick
import QtQml
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int sectionIndex: 2

    readonly property var sections: [
        {
            title: "Camera",
            icon: "camera"
        },
        {
            title: "Tracking",
            icon: "hand"
        },
        {
            title: "Control",
            icon: "target"
        },
        {
            title: "Gestures",
            icon: "mapping"
        },
        {
            title: "Model",
            icon: "settings"
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Settings"
            subtitle: "Camera, tracking, control and model configuration."
            VcButton {
                theme: root.theme
                text: "Reset defaults"
                iconName: "reset"
                implicitWidth: 128
                onClicked: root.uiState.resetSettingsDefaults()
            }
            VcButton {
                theme: root.theme
                text: "Save"
                iconName: "save"
                implicitWidth: 90
                enabled: false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.preferredWidth: 230
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6
                    Repeater {
                        model: root.sections
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: root.sectionIndex === index ? root.theme.surfaceHover : "transparent"
                            border.width: root.sectionIndex === index ? 1 : 0
                            border.color: root.theme.border
                            radius: root.theme.controlRadius

                            Rectangle {
                                visible: root.sectionIndex === index
                                anchors.left: parent.left
                                width: 2
                                height: parent.height
                                color: root.theme.accent
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 10
                                spacing: 11
                                VcIcon {
                                    Layout.preferredWidth: 21
                                    Layout.preferredHeight: 21
                                    name: modelData.icon
                                    color: root.sectionIndex === index ? root.theme.textPrimary : root.theme.textMuted
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: root.sectionIndex === index
                                        ? root.theme.textPrimary
                                        : root.theme.textSecondary
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 12
                                    font.weight: root.sectionIndex === index ? Font.DemiBold : Font.Normal
                                }
                                Text {
                                    text: "›"
                                    color: root.theme.textMuted
                                    font.pixelSize: 16
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.sectionIndex = index
                            }
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.borderSubtle
                    }
                    Text {
                        text: "Virtual Controller v" + Qt.application.version
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        Layout.leftMargin: 6
                    }
                }
            }

            VcCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme

                StackLayout {
                    anchors.fill: parent
                    currentIndex: root.sectionIndex

                    // CAMERA
                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0
                            Text {
                                text: "Camera"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Video source and preview behavior."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Camera device"
                                description: "Select the RGB camera used for hand tracking."
                                VcComboBox {
                                    theme: root.theme
                                    model: root.uiState.cameraNames
                                    currentIndex: root.uiState.cameraIndex
                                    enabled: !root.uiState.pipelineRunning
                                        && root.uiState.cameraNames.length > 0
                                    implicitWidth: 280

                                    onActivated: function(index) {
                                        root.uiState.selectCamera(index)
                                    }
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
                                title: "Capture format"
                                description: "Resolution and target frame rate."
                                VcComboBox {
                                    theme: root.theme
                                    model: root.uiState.formatNames
                                    currentIndex: root.uiState.formatIndex
                                    enabled: !root.uiState.pipelineRunning
                                        && root.uiState.formatNames.length > 0
                                    implicitWidth: 280

                                    onActivated: function(index) {
                                        root.uiState.selectFormat(index)
                                    }
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
                                title: "Mirror preview"
                                description: "Mirror only the displayed image; tracking coordinates remain normalized."
                                VcSwitch {
                                    id: mirrorPreviewSwitch
                                    theme: root.theme
                                    checked: root.uiState.mirrorPreview
                                    onToggled: root.uiState.mirrorPreview = checked
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
                                title: "Swap handedness"
                                description: "Swap logical LEFT / RIGHT if camera placement requires it."
                                VcSwitch {
                                    id: swapHandednessSwitch
                                    theme: root.theme
                                    checked: root.uiState.swapHandedness
                                    onToggled: root.uiState.setSwapHandedness(checked)
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // TRACKING
                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0
                            Text {
                                text: "Tracking"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Hand detection and landmark stability."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Maximum hands"
                                description: "The project architecture is designed for simultaneous " +
                                    "left and right hand tracking."
                                VcComboBox {
                                    theme: root.theme
                                    model: ["2 hands"]
                                    implicitWidth: 180
                                    enabled: false
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
                                title: "Detection confidence"
                                description: "Minimum confidence required to accept a newly detected hand."
                                VcSlider {
                                    id: detectionConfidenceSlider
                                    theme: root.theme
                                    value: root.uiState.detectionConfidence
                                    enabled: !root.uiState.pipelineRunning
                                    implicitWidth: 230
                                    onMoved: root.uiState.setDetectionConfidence(value)
                                }
                                Text {
                                    text: Math.round(detectionConfidenceSlider.value) + "%"
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Tracking confidence"
                                description: "Minimum confidence used to keep tracking an existing hand."
                                VcSlider {
                                    id: trackingConfidenceSlider
                                    theme: root.theme
                                    value: root.uiState.trackingConfidence
                                    enabled: !root.uiState.pipelineRunning
                                    implicitWidth: 230
                                    onMoved: root.uiState.setTrackingConfidence(value)
                                }
                                Text {
                                    text: Math.round(trackingConfidenceSlider.value) + "%"
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Show landmarks"
                                description: "Render landmarks and skeleton on the camera preview."
                                VcSwitch {
                                    id: showLandmarksSwitch
                                    theme: root.theme
                                    checked: root.uiState.showLandmarks
                                    onToggled: root.uiState.showLandmarks = checked
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // CONTROL
                    Item {
                        enabled: root.uiState.cursorControlAvailable
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0
                            Text {
                                text: "Control"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Cursor response, smoothing and deadzone."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Sensitivity"
                                description: "Fine cursor gain for normal hand movement."
                                VcSlider {
                                    id: settingsSensitivitySlider
                                    theme: root.theme
                                    value: root.uiState.sensitivity
                                    implicitWidth: 230
                                    onMoved: root.uiState.setSensitivity(value)
                                }
                                Text {
                                    text: Math.round(settingsSensitivitySlider.value) + "%"
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Cursor speed"
                                description: "DPI-like multiplier. Higher values move across the monitor with less hand travel."
                                VcSlider {
                                    id: cursorSpeedSlider
                                    theme: root.theme
                                    from: 50
                                    to: 400
                                    stepSize: 10
                                    value: root.uiState.cursorSpeedPercent
                                    implicitWidth: 230
                                    onMoved: root.uiState.setCursorSpeedPercent(value)
                                }
                                Text {
                                    text: (cursorSpeedSlider.value / 100.0).toFixed(1) + "x"
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Invert X axis"
                                description: "Reverse horizontal cursor direction."
                                VcSwitch {
                                    id: invertXSwitch
                                    theme: root.theme
                                    checked: root.uiState.invertX
                                    onToggled: root.uiState.setInvertX(checked)
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
                                title: "Invert Y axis"
                                description: "Reverse vertical cursor direction."
                                VcSwitch {
                                    id: invertYSwitch
                                    theme: root.theme
                                    checked: root.uiState.invertY
                                    onToggled: root.uiState.setInvertY(checked)
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
                                title: "Smoothing"
                                description: "Apply the One Euro filter to reduce cursor jitter."
                                VcSwitch {
                                    id: smoothingSwitch
                                    theme: root.theme
                                    checked: root.uiState.smoothing
                                    onToggled: root.uiState.setSmoothing(checked)
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
                                title: "Deadzone"
                                description: "Suppress very small per-frame hand movement before it reaches the cursor."
                                VcSlider {
                                    id: deadzoneSlider
                                    theme: root.theme
                                    value: root.uiState.deadzone
                                    implicitWidth: 230
                                    onMoved: root.uiState.setDeadzone(value)
                                }
                                Text {
                                    text: Math.round(deadzoneSlider.value) + "%"
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Active profile"
                                description: "Profiles combine mappings with control tuning."
                                VcComboBox {
                                    id: settingsProfileCombo
                                    theme: root.theme
                                    model: root.uiState.profileNames
                                    currentIndex: Math.max(0, root.uiState.profileNames.indexOf(root.uiState.activeProfile))
                                    implicitWidth: 240
                                    onActivated: root.uiState.activeProfile = currentText
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // GESTURES
                    Item {
                        enabled: root.uiState.gestureRecognitionAvailable
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0
                            Text {
                                text: "Gestures"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Gesture event thresholds and state behavior."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Recognition threshold"
                                description: "Minimum recognition confidence before a gesture is reported as active."
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
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Debounce (M4)"
                                description: "Reserved for PRESS/HOLD/RELEASE event generation in M4."
                                VcComboBox {
                                    id: debounceCombo
                                    theme: root.theme
                                    enabled: false
                                    model: ["80 ms", "120 ms", "160 ms", "200 ms"]
                                    currentIndex: Math.max(0, [80, 120, 160, 200].indexOf(root.uiState.debounceMs))
                                    implicitWidth: 180
                                    onActivated: root.uiState.debounceMs = parseInt(currentText)
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
                                title: "Cooldown (M4)"
                                description: "Reserved for discrete gesture actions in M4."
                                VcComboBox {
                                    id: cooldownCombo
                                    theme: root.theme
                                    enabled: false
                                    model: ["150 ms", "250 ms", "400 ms", "600 ms"]
                                    currentIndex: Math.max(0, [150, 250, 400, 600].indexOf(root.uiState.cooldownMs))
                                    implicitWidth: 180
                                    onActivated: root.uiState.cooldownMs = parseInt(currentText)
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
                                title: "Require release (M4)"
                                description: "Reserved for the gesture event state machine in M4."
                                VcSwitch {
                                    id: requireReleaseSwitch
                                    theme: root.theme
                                    enabled: false
                                    checked: root.uiState.requireRelease
                                    onToggled: root.uiState.requireRelease = checked
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // MODEL
                    Item {
                        enabled: root.uiState.gestureRecognitionAvailable
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0
                            Text {
                                text: "Model"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Gesture recognition backend and optional causal TCN runtime."
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: "Hand tracker"
                                description: "MediaPipe Hand Landmarker runtime model."
                                VcTextField {
                                    theme: root.theme
                                    text: "models/hand_landmarker.task"
                                    implicitWidth: 300
                                    readOnly: true
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
                                title: "Gesture model"
                                description: "Causal TCN exported to ONNX and executed by ONNX Runtime C++."
                                VcTextField {
                                    theme: root.theme
                                    text: "models/gesture_model.onnx"
                                    implicitWidth: 300
                                    readOnly: true
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
                                title: "Recognition backend"
                                description: root.uiState.gestureModelStatus
                                StatusBadge {
                                    theme: root.theme
                                    text: root.uiState.gestureBackendName
                                    kind: root.uiState.gestureModelActive ? "success" : "neutral"
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
                                title: "Reload model (later)"
                                description: "Restart tracking after replacing the model. Hot reload is not enabled yet."
                                VcButton {
                                    theme: root.theme
                                    text: "Reload"
                                    iconName: "reset"
                                    implicitWidth: 110
                                    enabled: false
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }
                }
            }
        }
    }
    Connections {
        target: root.uiState

        function onSensitivityChanged() {
            if (!settingsSensitivitySlider.pressed)
                settingsSensitivitySlider.value = root.uiState.sensitivity
        }

        function onCursorSpeedPercentChanged() {
            if (!cursorSpeedSlider.pressed)
                cursorSpeedSlider.value = root.uiState.cursorSpeedPercent
        }

        function onInvertXChanged() {
            if (invertXSwitch.checked !== root.uiState.invertX)
                invertXSwitch.checked = root.uiState.invertX
        }

        function onInvertYChanged() {
            if (invertYSwitch.checked !== root.uiState.invertY)
                invertYSwitch.checked = root.uiState.invertY
        }

        function onDeadzoneChanged() {
            if (!deadzoneSlider.pressed)
                deadzoneSlider.value = root.uiState.deadzone
        }

        function onDetectionConfidenceChanged() {
            if (!detectionConfidenceSlider.pressed)
                detectionConfidenceSlider.value = root.uiState.detectionConfidence
        }

        function onTrackingConfidenceChanged() {
            if (!trackingConfidenceSlider.pressed)
                trackingConfidenceSlider.value = root.uiState.trackingConfidence
        }

        function onRecognitionThresholdChanged() {
            if (!recognitionThresholdSlider.pressed)
                recognitionThresholdSlider.value = root.uiState.recognitionThreshold
        }

        function onSmoothingChanged() {
            if (smoothingSwitch.checked !== root.uiState.smoothing)
                smoothingSwitch.checked = root.uiState.smoothing
        }

        function onMirrorPreviewChanged() {
            if (mirrorPreviewSwitch.checked !== root.uiState.mirrorPreview)
                mirrorPreviewSwitch.checked = root.uiState.mirrorPreview
        }

        function onSwapHandednessChanged() {
            if (swapHandednessSwitch.checked !== root.uiState.swapHandedness)
                swapHandednessSwitch.checked = root.uiState.swapHandedness
        }

        function onShowLandmarksChanged() {
            if (showLandmarksSwitch.checked !== root.uiState.showLandmarks)
                showLandmarksSwitch.checked = root.uiState.showLandmarks
        }

        function onRequireReleaseChanged() {
            if (requireReleaseSwitch.checked !== root.uiState.requireRelease)
                requireReleaseSwitch.checked = root.uiState.requireRelease
        }

        function onActiveProfileChanged() {
            const index = root.uiState.profileNames.indexOf(root.uiState.activeProfile)
            if (index >= 0 && settingsProfileCombo.currentIndex !== index)
                settingsProfileCombo.currentIndex = index
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
