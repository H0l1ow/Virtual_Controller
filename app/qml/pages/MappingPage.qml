import QtQuick
import QtQml
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState

    property int selectedRow: uiState.mappingRows.length > 0 ? 0 : -1
    readonly property var selectedMapping:
        selectedRow >= 0 && uiState.mappingRows.length > selectedRow
            ? uiState.mappingRows[selectedRow]
            : ({})

    readonly property var handOptions: ["Left", "Right"]
    readonly property var gestureOptions: uiState.mappingGestureOptions
    readonly property bool selectedGestureAvailable:
        gestureCombo.currentText.length > 0
            && uiState.gestureRuntimeAvailable(gestureCombo.currentText)
    readonly property var behaviorOptions: ["Press", "Hold", "Toggle"]
    readonly property var actionOptions: [
        "Left click", "Right click", "Scroll up", "Scroll down", "Freeze cursor",
        "Key Space", "Key Enter", "Key Escape", "Key Tab",
        "Key Left", "Key Right", "Key Up", "Key Down",
        "Key Ctrl", "Key Shift", "Key Alt"
    ]

    function optionIndex(options, value, fallback) {
        const index = options.indexOf(value)
        return index >= 0 ? index : fallback
    }

    function syncEditor() {
        if (root.selectedRow < 0 || root.uiState.mappingRows.length <= root.selectedRow) {
            handCombo.currentIndex = 1
            gestureCombo.currentIndex = 0
            actionCombo.currentIndex = 0
            behaviorCombo.currentIndex = 1
            enabledSwitch.checked = false
            return
        }

        const mapping = root.uiState.mappingRows[root.selectedRow]
        handCombo.currentIndex = root.optionIndex(root.handOptions, mapping.hand || "Right", 1)
        gestureCombo.currentIndex = root.optionIndex(root.gestureOptions, mapping.source || "PINCH", 0)
        actionCombo.currentIndex = root.optionIndex(root.actionOptions, mapping.action || "Left click", 0)
        behaviorCombo.currentIndex = root.optionIndex(root.behaviorOptions, mapping.behavior || "Hold", 1)
        enabledSwitch.checked = mapping.enabled === true
    }

    onSelectedRowChanged: Qt.callLater(root.syncEditor)
    Component.onCompleted: Qt.callLater(root.syncEditor)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Gesture Mapping"
            subtitle: "M4: PRESS / HOLD / RELEASE events mapped to mouse and keyboard actions."

            VcComboBox {
                id: mappingProfileCombo
                theme: root.theme
                model: root.uiState.profileNames
                currentIndex: Math.max(0, root.uiState.profileNames.indexOf(root.uiState.activeProfile))
                implicitWidth: 160
                onActivated: root.uiState.setActiveProfile(currentText)
            }

            VcButton {
                theme: root.theme
                text: "Reload"
                iconName: "reset"
                implicitWidth: 100
                onClicked: root.uiState.reloadActiveProfile()
            }

            VcButton {
                theme: root.theme
                text: "Add mapping"
                iconName: "plus"
                implicitWidth: 124
                onClicked: root.selectedRow = root.uiState.addMapping()
            }

            VcButton {
                theme: root.theme
                text: "Remove"
                iconName: "trash"
                variant: "danger"
                implicitWidth: 104
                enabled: root.selectedRow >= 0
                onClicked: {
                    const oldIndex = root.selectedRow
                    root.uiState.removeMapping(oldIndex)
                    root.selectedRow = Math.min(oldIndex, root.uiState.mappingRows.length - 1)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.uiState.mappingError.length > 0
            text: root.uiState.mappingError
            color: root.theme.error
            font.family: root.theme.fontFamily
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.preferredWidth: 210
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "mapping"
                        title: "Event model"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 14
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            text: "Recognized gesture"
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "↓  debounce"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        StatusBadge { theme: root.theme; text: "PRESS"; kind: "success" }
                        StatusBadge { theme: root.theme; text: "HOLD"; kind: "neutral" }
                        StatusBadge { theme: root.theme; text: "RELEASE"; kind: "warning" }
                        Text {
                            Layout.fillWidth: true
                            text: "↓  ActionMapper"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Mouse / Keyboard"
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: 6
                            color: root.theme.borderSubtle
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Profiles"
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "JSON profiles are stored in:\n" + root.uiState.mappingProfileDirectory
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

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
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            text: "Mappings"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: root.uiState.mappingRows.length + " rules"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        color: root.theme.backgroundAlt

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Repeater {
                                model: [
                                    { title: "Hand", width: 58 },
                                    { title: "Gesture", width: 116 },
                                    { title: "Behavior", width: 76 },
                                    { title: "Action", width: 130 },
                                    { title: "Output", width: 78 },
                                    { title: "State", width: 76 }
                                ]
                                delegate: Text {
                                    required property var modelData
                                    Layout.preferredWidth: modelData.width
                                    Layout.fillWidth: modelData.title === "Action"
                                    text: modelData.title.toUpperCase()
                                    color: root.theme.textMuted
                                    font.family: root.theme.fontFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: 0.6
                                }
                            }
                        }
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: width
                        contentHeight: mappingColumn.implicitHeight
                        clip: true

                        Column {
                            id: mappingColumn
                            width: parent.width

                            Repeater {
                                model: root.uiState.mappingRows

                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: mappingColumn.width
                                    height: 56
                                    color: root.selectedRow === index
                                        ? root.theme.surfaceHover
                                        : "transparent"

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        height: 1
                                        color: root.theme.borderSubtle
                                    }
                                    Rectangle {
                                        visible: root.selectedRow === index
                                        anchors.left: parent.left
                                        width: 2
                                        height: parent.height
                                        color: root.theme.accent
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 8

                                        Text {
                                            Layout.preferredWidth: 58
                                            text: modelData.hand
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        Text {
                                            Layout.preferredWidth: 116
                                            text: modelData.source
                                            color: root.theme.textPrimary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                        }
                                        TagChip {
                                            Layout.preferredWidth: 76
                                            theme: root.theme
                                            text: modelData.behavior
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 130
                                            text: modelData.action
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.preferredWidth: 78
                                            text: modelData.output
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        StatusBadge {
                                            Layout.preferredWidth: 76
                                            theme: root.theme
                                            text: modelData.state
                                            kind: modelData.enabled ? "success" : "neutral"
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.selectedRow = index
                                    }
                                }
                            }
                        }
                    }
                }
            }

            VcCard {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        iconName: "settings"
                        title: "Mapping Details"
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.border
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 14
                        spacing: 9
                        enabled: root.selectedRow >= 0

                        Text { text: "Hand"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: handCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.handOptions
                        }

                        Text { text: "Gesture"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: gestureCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.gestureOptions
                            onActivated: {
                                if (!root.selectedGestureAvailable)
                                    enabledSwitch.checked = false
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: gestureCombo.currentText.length > 0
                                && !root.selectedGestureAvailable
                            text: "Temporal gesture reserved for the next recognition stage. It can be stored disabled, but cannot emit actions yet."
                            color: root.theme.warning
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                        }

                        Text { text: "Action"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: actionCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.actionOptions
                            onActivated: {
                                if (currentText === "Freeze cursor"
                                        && behaviorCombo.currentText === "Press") {
                                    behaviorCombo.currentIndex = root.behaviorOptions.indexOf("Hold")
                                }
                            }
                        }

                        Text { text: "Behavior"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: behaviorCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.behaviorOptions
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: 3
                            color: root.theme.borderSubtle
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: "Enabled"
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                            VcSwitch {
                                id: enabledSwitch
                                theme: root.theme
                                checked: false
                                enabled: root.selectedGestureAvailable
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: actionCombo.currentText === "Freeze cursor"
                                ? (behaviorCombo.currentText === "Toggle"
                                    ? "Toggle freezes/unfreezes cursor movement on each PRESS. Tracking continues in the background."
                                    : behaviorCombo.currentText === "Hold"
                                        ? "Hold freezes cursor movement until RELEASE. You can reposition your hand without a cursor jump."
                                        : "Press freezes cursor movement for the PRESS frame only; Hold is recommended.")
                                : behaviorCombo.currentText === "Hold"
                                    ? "Hold keeps the button/key down until RELEASE. Useful for pinch-drag."
                                    : behaviorCombo.currentText === "Toggle"
                                        ? "Toggle flips the target state on every PRESS. F8 or disarm always releases it."
                                        : "Press generates a short one-shot action when PRESS is emitted."
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                        }

                        Item { Layout.fillHeight: true }

                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: "Save mapping"
                            iconName: "save"
                            onClicked: root.uiState.updateMapping(
                                root.selectedRow,
                                handCombo.currentText,
                                gestureCombo.currentText,
                                actionCombo.currentText,
                                behaviorCombo.currentText,
                                enabledSwitch.checked)
                        }

                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: "Remove mapping"
                            variant: "danger"
                            onClicked: {
                                const oldIndex = root.selectedRow
                                root.uiState.removeMapping(oldIndex)
                                root.selectedRow = Math.min(oldIndex, root.uiState.mappingRows.length - 1)
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: root.uiState

        function onActiveProfileChanged() {
            const index = root.uiState.profileNames.indexOf(root.uiState.activeProfile)
            if (index >= 0 && mappingProfileCombo.currentIndex !== index)
                mappingProfileCombo.currentIndex = index
            root.selectedRow = root.uiState.mappingRows.length > 0 ? 0 : -1
        }

        function onMappingRowsChanged() {
            if (root.uiState.mappingRows.length === 0)
                root.selectedRow = -1
            else if (root.selectedRow < 0 || root.selectedRow >= root.uiState.mappingRows.length)
                root.selectedRow = 0
            Qt.callLater(root.syncEditor)
        }
    }
}
