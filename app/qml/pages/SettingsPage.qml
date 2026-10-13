import VirtualController.I18n 1.0
import VirtualController.Preferences 1.0
import VirtualController.Desktop 1.0
import QtQuick
import QtQml
import QtQuick.Layouts
import QtQuick.Controls
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int sectionIndex: 0
    readonly property var shortcutValues: ["Disabled", "Ctrl+Alt+F5", "Ctrl+Alt+F6",
        "Ctrl+Alt+F7", "Ctrl+Alt+F9", "Ctrl+Alt+F10", "Ctrl+Alt+F11", "Ctrl+Alt+F12"]
    readonly property var shortcutLabels: [qsTr("Disabled"), "Ctrl+Alt+F5", "Ctrl+Alt+F6",
        "Ctrl+Alt+F7", "Ctrl+Alt+F9", "Ctrl+Alt+F10", "Ctrl+Alt+F11", "Ctrl+Alt+F12"]

    readonly property var sections: [
        {
            title: qsTr("General"),
            icon: "settings"
        },
        {
            title: qsTr("Camera"),
            icon: "camera"
        },
        {
            title: qsTr("Tracking"),
            icon: "hand"
        },
        {
            title: qsTr("Cursor"),
            icon: "target"
        },
        {
            title: qsTr("Desktop"),
            icon: "monitor"
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Settings")
            subtitle: qsTr("Application, camera, cursor and desktop preferences.")
            Text {
                text: root.uiState.settingsSaveError.length > 0
                    ? qsTr("Save failed")
                    : root.uiState.hasUnsavedSettings
                        ? qsTr("Unsaved changes")
                        : qsTr("Saved")
                color: root.uiState.settingsSaveError.length > 0
                    ? root.theme.error
                    : root.uiState.hasUnsavedSettings
                        ? root.theme.warning : root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pixelSize: 11
            }
            VcButton {
                theme: root.theme
                text: qsTr("Reset defaults")
                iconName: "reset"
                enabled: !root.uiState.pipelineRunning
                onClicked: root.uiState.resetSettingsDefaults()
            }
            VcButton {
                theme: root.theme
                text: qsTr("Save")
                iconName: "save"
                enabled: root.uiState.hasUnsavedSettings
                onClicked: root.uiState.saveSettings()
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
                        text: qsTr("Virtual Controller v") + Qt.application.version
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

                    // GENERAL / LOCALIZATION
                    Item {
                        ScrollView {
                            id: generalScroll
                            anchors.fill: parent
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            contentHeight: generalColumn.implicitHeight + 36
                            ColumnLayout {
                                id: generalColumn
                                x: 18
                                y: 18
                                width: Math.max(1, generalScroll.availableWidth - 36)
                                spacing: 0

                            Text {
                                text: qsTr("General")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: qsTr("Application language and appearance.")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Language")
                                description: qsTr("Change the interface language instantly. Your choice is saved automatically.")
                                VcComboBox {
                                    id: languageCombo
                                    theme: root.theme
                                    model: LanguageManager.languageNames
                                    currentIndex: LanguageManager.languageIndex
                                    implicitWidth: 220
                                    onActivated: function(index) {
                                        LanguageManager.setLanguageByIndex(index)
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
                                title: qsTr("Appearance")
                                description: qsTr("Use the system theme or switch between light and dark mode.")
                                VcComboBox {
                                    theme: root.theme
                                    model: [qsTr("Follow Windows"), qsTr("Dark"), qsTr("Light")]
                                    currentIndex: Math.max(0, ["system", "dark", "light"].indexOf(AppPreferences.themeMode))
                                    implicitWidth: 220
                                    onActivated: function(index) {
                                        AppPreferences.themeMode = ["system", "dark", "light"][index]
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.topMargin: 14
                                text: qsTr("Settings take effect immediately. Click Save to keep them after a restart.")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                            }
                            Item { Layout.fillHeight: true }
                            }
                        }
                    }

                    // CAMERA
                    Item {
                        ScrollView {
                            id: cameraScroll
                            anchors.fill: parent
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            contentHeight: cameraColumn.implicitHeight + 36
                            ColumnLayout {
                                id: cameraColumn
                                x: 18
                                y: 18
                                width: Math.max(1, cameraScroll.availableWidth - 36)
                                spacing: 0
                            Text {
                                text: qsTr("Camera")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: qsTr("Video source and preview behavior.")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Camera device")
                                description: qsTr("Select the RGB camera used for hand tracking.")
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
                                title: qsTr("Capture format")
                                description: qsTr("Resolution and target frame rate.")
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
                                title: qsTr("Camera auto-reconnect")
                                description: qsTr("Retry for 30 seconds after camera disconnection. System input remains disarmed.")
                                VcSwitch {
                                    theme: root.theme
                                    checked: root.uiState.autoRecoverCamera
                                    onToggled: root.uiState.setAutoRecoverCamera(checked)
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.theme.borderSubtle
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: root.uiState.cameraRecoveryStatus.length > 0
                                text: root.uiState.cameraRecoveryStatus === "Waiting for camera reconnection"
                                    ? qsTr("Waiting for camera reconnection")
                                    : root.uiState.cameraRecoveryStatus === "Camera reconnection timed out"
                                        ? qsTr("Camera reconnection timed out")
                                        : root.uiState.cameraRecoveryStatus === "Camera reconnected; output disarmed"
                                            ? qsTr("Camera reconnected; output disarmed")
                                            : qsTr("Camera unavailable")
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                color: root.theme.warning
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Mirror preview")
                                description: qsTr("Mirror only the displayed image; tracking coordinates remain normalized.")
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
                                title: qsTr("Swap handedness")
                                description: qsTr("Swap logical LEFT / RIGHT if camera placement requires it.")
                                VcSwitch {
                                    id: swapHandednessSwitch
                                    theme: root.theme
                                    checked: root.uiState.swapHandedness
                                    onToggled: root.uiState.setSwapHandedness(checked)
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.topMargin: 20
                                Layout.bottomMargin: 10
                                text: qsTr("Mini preview (PiP)")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.theme.borderSubtle
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Mini camera preview")
                                description: qsTr("Show a camera overlay in the bottom-right corner when minimized.")
                                VcSwitch {
                                    theme: root.theme
                                    checked: AppPreferences.pipEnabled
                                    onToggled: AppPreferences.pipEnabled = checked
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
                                title: qsTr("Mini preview size")
                                description: qsTr("Set the size of the preview shown when minimized.")
                                VcComboBox {
                                    theme: root.theme
                                    model: [qsTr("Small"), qsTr("Medium"), qsTr("Large")]
                                    currentIndex: Math.max(0, ["small", "medium", "large"].indexOf(AppPreferences.pipSizeMode))
                                    implicitWidth: 220
                                    onActivated: function(index) {
                                        AppPreferences.pipSizeMode = ["small", "medium", "large"][index]
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
                                title: qsTr("Mini preview monitor")
                                description: qsTr("Choose a display; if disconnected, use the primary display.")
                                VcComboBox {
                                    theme: root.theme
                                    model: [qsTr("Primary monitor")].concat(AppPreferences.pipScreenNames)
                                    currentIndex: Math.max(0, AppPreferences.pipScreenNames.indexOf(AppPreferences.pipScreenName) + 1)
                                    implicitWidth: Math.max(210, contentItem.implicitWidth + 35)
                                    onActivated: function(index) {
                                        AppPreferences.pipScreenName = index === 0 ? "" : AppPreferences.pipScreenNames[index - 1]
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
                                title: qsTr("Show landmarks in mini preview")
                                description: qsTr("Display tracked hands over the camera image in the small window.")
                                VcSwitch {
                                    theme: root.theme
                                    checked: AppPreferences.pipShowLandmarks
                                    onToggled: AppPreferences.pipShowLandmarks = checked
                                }
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Mini preview position")
                                description: qsTr("Reset the preview to the bottom-right corner of the screen.")
                                VcButton {
                                    theme: root.theme
                                    text: qsTr("Reset position")
                                    iconName: "reset"
                                    onClicked: AppPreferences.setPipPosition(-1, -1)
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.theme.borderSubtle
                            }
                            Item { Layout.fillHeight: true }
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
                                text: qsTr("Tracking")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: qsTr("Hand detection and landmark stability.")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Maximum hands")
                                description: qsTr("The project architecture is designed for simultaneous ") +
                                    qsTr("left and right hand tracking.")
                                VcComboBox {
                                    theme: root.theme
                                    model: [qsTr("2 hands")]
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
                                title: qsTr("Detection confidence")
                                description: qsTr("Minimum confidence required to accept a newly detected hand.")
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
                                title: qsTr("Tracking confidence")
                                description: qsTr("Minimum confidence used to keep tracking an existing hand.")
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
                                title: qsTr("Show landmarks")
                                description: qsTr("Render landmarks and skeleton on the camera preview.")
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
                                text: qsTr("Cursor")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: qsTr("Cursor response, smoothing and deadzone.")
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                Layout.bottomMargin: 16
                            }
                            SettingsRow {
                                Layout.fillWidth: true
                                theme: root.theme
                                title: qsTr("Sensitivity")
                                description: qsTr("Fine cursor gain for normal hand movement.")
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
                                title: qsTr("Cursor speed")
                                description: qsTr("DPI-like multiplier. Higher values move across the monitor with less hand travel.")
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
                                title: qsTr("Invert X axis")
                                description: qsTr("Reverse horizontal cursor direction.")
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
                                title: qsTr("Invert Y axis")
                                description: qsTr("Reverse vertical cursor direction.")
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
                                title: qsTr("Smoothing")
                                description: qsTr("Apply the One Euro filter to reduce cursor jitter.")
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
                                title: qsTr("Deadzone")
                                description: qsTr("Suppress very small per-frame hand movement before it reaches the cursor.")
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

                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }


                    // DESKTOP INTEGRATION / GLOBAL SHORTCUTS (M5.4)
                    Item {
                        ScrollView {
                            id: desktopScroll
                            anchors.fill: parent
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            contentHeight: desktopColumn.implicitHeight + 36
                            ColumnLayout {
                                id: desktopColumn
                                x: 18
                                y: 18
                                width: Math.max(1, desktopScroll.availableWidth - 36)
                                spacing: 10

                                Text {
                                    text: qsTr("Desktop integration")
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 20
                                    font.weight: Font.DemiBold
                                    color: root.theme.textPrimary
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Global shortcuts work while minimized on Windows. F8 remains the independent emergency release key.")
                                    color: root.theme.textMuted
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    Layout.bottomMargin: 8
                                }
                                SettingsRow {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    title: qsTr("System tray")
                                    description: DesktopIntegration.trayAvailable
                                        ? qsTr("Right-click the tray icon for tracking, profile and exit actions.")
                                        : qsTr("System tray is unavailable in this environment.")
                                    StatusBadge {
                                        theme: root.theme
                                        text: DesktopIntegration.trayAvailable ? qsTr("Available") : qsTr("Unavailable")
                                        kind: DesktopIntegration.trayAvailable ? "success" : "warning"
                                    }
                                }
                                SettingsRow {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    title: qsTr("Toggle tracking")
                                    description: qsTr("Starts or stops camera tracking; never arms system input.")
                                    VcComboBox {
                                        theme: root.theme
                                        enabled: DesktopIntegration.globalShortcutsAvailable
                                        implicitWidth: 220
                                        model: root.shortcutLabels
                                        currentIndex: Math.max(0, root.shortcutValues.indexOf(AppPreferences.shortcutTracking))
                                        onActivated: function(index) {
                                            AppPreferences.shortcutTracking = root.shortcutValues[index]
                                        }
                                    }
                                }
                                SettingsRow {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    title: qsTr("Stop and disarm")
                                    description: qsTr("Releases all system inputs and stops tracking. F8 is always available independently.")
                                    VcComboBox {
                                        theme: root.theme
                                        enabled: DesktopIntegration.globalShortcutsAvailable
                                        implicitWidth: 220
                                        model: root.shortcutLabels
                                        currentIndex: Math.max(0, root.shortcutValues.indexOf(AppPreferences.shortcutStop))
                                        onActivated: function(index) {
                                            AppPreferences.shortcutStop = root.shortcutValues[index]
                                        }
                                    }
                                }
                                SettingsRow {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    title: qsTr("Next profile")
                                    description: qsTr("Changes to the next profile and disarms system input.")
                                    VcComboBox {
                                        theme: root.theme
                                        enabled: DesktopIntegration.globalShortcutsAvailable
                                        implicitWidth: 220
                                        model: root.shortcutLabels
                                        currentIndex: Math.max(0, root.shortcutValues.indexOf(AppPreferences.shortcutNextProfile))
                                        onActivated: function(index) {
                                            AppPreferences.shortcutNextProfile = root.shortcutValues[index]
                                        }
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    visible: DesktopIntegration.shortcutError.length > 0
                                    text: DesktopIntegration.shortcutError
                                    color: root.theme.warning
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Changes take effect immediately. Click Save to persist them. If Windows or another application owns a shortcut, choose another combination.")
                                    color: root.theme.textMuted
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                }
                                Item { Layout.preferredHeight: 12 }
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


    }

}
