import VirtualController.I18n 1.0
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

    function mappingErrorText(message) {
        // C++ errors use stable English identifiers; translate only their UI
        // representation, leaving profile JSON values untouched.
        LanguageManager.language
        return qsTr(message)
    }

    function issueDescription(issue) {
        LanguageManager.language
        switch (issue.code) {
        case "duplicate_id": return qsTr("Two mapping rows use the same internal ID. System input is blocked until fixed.")
        case "scroll_toggle": return qsTr("Scroll actions cannot use Toggle. Use Press or Hold.")
        case "cursor_press": return qsTr("Enable cursor movement requires Hold or Toggle.")
        case "duplicate_action": return qsTr("Two enabled mappings generate the same action from one gesture.")
        case "conflicting_behavior": return qsTr("The same hand, gesture and action have conflicting behaviors.")
        case "cursor_gate_freeze": return qsTr("This gesture both enables and freezes cursor movement.")
        case "drag_exclusive": return qsTr("One hand cannot hold different gestures for cursor movement and drag simultaneously.")
        case "no_cursor_gate": return qsTr("No enabled cursor movement gate: Windows cursor remains locked.")
        default: return qsTr("Potential mapping conflict.")
        }
    }

    readonly property var handOptions: ["Left", "Right"]
    readonly property var gestureOptions: ["PINCH", "FIST", "POINT", "OPEN HAND"]
    readonly property var behaviorOptions: ["Press", "Hold", "Toggle"]
    readonly property var actionOptions: [
        "Left click", "Right click", "Scroll up", "Scroll down", "Freeze cursor",
        "Enable cursor movement",
        "Key Space", "Key Enter", "Key Escape", "Key Tab",
        "Key Left", "Key Right", "Key Up", "Key Down",
        "Key Ctrl", "Key Shift", "Key Alt"
    ]

    function displayOption(value) {
        // The mapper expects stable English action/behavior identifiers.
        // Only the ComboBox and table labels are translated.
        LanguageManager.language
        return qsTr(value)
    }

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
            title: qsTr("Gesture Mapping")
            subtitle: qsTr("Map gesture events to mouse/keyboard actions; conflicts are checked live.")

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
                text: qsTr("Reload")
                iconName: "reset"
                onClicked: root.uiState.reloadActiveProfile()
            }

            VcButton {
                theme: root.theme
                text: qsTr("Add mapping")
                iconName: "plus"
                onClicked: root.selectedRow = root.uiState.addMapping()
            }

            VcButton {
                theme: root.theme
                text: qsTr("Remove")
                iconName: "trash"
                variant: "danger"
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
            text: root.mappingErrorText(root.uiState.mappingError)
            color: root.theme.error
            font.family: root.theme.fontFamily
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }

        Rectangle {
            id: mappingIssuesPanel
            Layout.fillWidth: true
            Layout.preferredHeight: issueItems.implicitHeight + 12
            visible: root.uiState.mappingIssues.length > 0
            color: root.theme.surfaceRaised
            border.width: 1
            border.color: root.theme.borderStrong
            radius: root.theme.controlRadius
            ColumnLayout {
                id: issueItems
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                spacing: 4
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Mapping validation") + " · " + root.uiState.mappingIssues.length
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Repeater {
                    model: root.uiState.mappingIssues.slice(0, 3)
                    delegate: Text {
                        required property var modelData
                        Layout.fillWidth: true
                        text: (modelData.first >= 0 ? qsTr("Row %1: ").arg(modelData.first + 1) : "")
                            + root.issueDescription(modelData)
                        color: modelData.severity === "error" ? root.theme.error : root.theme.warning
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.uiState.mappingIssues.length > 3
                    text: qsTr("More warnings are available after correcting the displayed conflicts.")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }
            }
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
                        title: qsTr("Event model")
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
                            text: qsTr("Recognized gesture")
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
                            text: qsTr("Mouse / Keyboard")
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
                            text: qsTr("Profiles")
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("JSON profiles are stored in:\n") + root.uiState.mappingProfileDirectory
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                        }
                        Item { Layout.fillHeight: true }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: qsTr("Duplicate profile")
                            iconName: "plus"
                            onClicked: duplicateDialog.open()
                        }
                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: qsTr("Delete profile")
                            iconName: "trash"
                            variant: "danger"
                            enabled: root.uiState.canDeleteActiveProfile
                            onClicked: deleteDialog.open()
                        }
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
                            text: qsTr("Mappings")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: qsTr("Mappings") + ": " + root.uiState.mappingRows.length
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
                                    { title: qsTr("Hand"), width: 58 },
                                    { title: qsTr("Gesture"), width: 116 },
                                    { title: qsTr("Behavior"), width: 76 },
                                    { title: qsTr("Action"), width: 130 },
                                    { title: qsTr("Output"), width: 78 },
                                    { title: qsTr("State"), width: 76 }
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
                                            text: root.displayOption(modelData.hand)
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        Text {
                                            Layout.preferredWidth: 116
                                            text: root.displayOption(modelData.source)
                                            color: root.theme.textPrimary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                        }
                                        TagChip {
                                            Layout.preferredWidth: 76
                                            theme: root.theme
                                            text: root.displayOption(modelData.behavior)
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 130
                                            text: root.displayOption(modelData.action)
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.preferredWidth: 78
                                            text: root.displayOption(modelData.output)
                                            color: root.theme.textSecondary
                                            font.family: root.theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                        StatusBadge {
                                            Layout.preferredWidth: 76
                                            theme: root.theme
                                            text: root.displayOption(modelData.state)
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
                        title: qsTr("Mapping Details")
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

                        Text { text: qsTr("Hand"); color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: handCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.handOptions
                            labelFormatter: root.displayOption
                        }

                        Text { text: qsTr("Gesture"); color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: gestureCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.gestureOptions
                            labelFormatter: root.displayOption
                        }

                        Text { text: qsTr("Action"); color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: actionCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.actionOptions
                            labelFormatter: root.displayOption
                            onActivated: {
                                if ((currentText === "Freeze cursor"
                                        || currentText === "Enable cursor movement")
                                        && behaviorCombo.currentText === "Press") {
                                    behaviorCombo.currentIndex = root.behaviorOptions.indexOf("Hold")
                                }
                            }
                        }

                        Text { text: qsTr("Behavior"); color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        VcComboBox {
                            id: behaviorCombo
                            Layout.fillWidth: true
                            theme: root.theme
                            model: root.behaviorOptions
                            labelFormatter: root.displayOption
                            onActivated: {
                                if (actionCombo.currentText === "Enable cursor movement"
                                        && currentText === "Press") {
                                    currentIndex = root.behaviorOptions.indexOf("Hold")
                                }
                            }
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
                                text: qsTr("Enabled")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                            VcSwitch {
                                id: enabledSwitch
                                theme: root.theme
                                checked: false
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: actionCombo.currentText === "Freeze cursor"
                                ? (behaviorCombo.currentText === "Toggle"
                                    ? qsTr("Toggle freezes/unfreezes cursor movement on each PRESS. Tracking continues in the background.")
                                    : behaviorCombo.currentText === "Hold"
                                        ? qsTr("Hold freezes cursor movement until RELEASE. You can reposition your hand without a cursor jump.")
                                        : qsTr("Press freezes cursor movement for the PRESS frame only; Hold is recommended."))
                                : actionCombo.currentText === "Enable cursor movement"
                                    ? (behaviorCombo.currentText === "Toggle"
                                        ? qsTr("Toggle: first PRESS enables cursor movement, the next PRESS disables it. Cursor movement is locked by default and only an active Enable cursor movement mapping can unlock it.")
                                        : qsTr("Hold: cursor movement is enabled only while this gesture is held. Tracking continues while locked, so activating the gesture does not cause a cursor jump."))
                                    : behaviorCombo.currentText === "Hold"
                                        ? qsTr("Hold keeps the button/key down until RELEASE. Useful for pinch-drag.")
                                        : behaviorCombo.currentText === "Toggle"
                                            ? qsTr("Toggle flips the target state on every PRESS. F8 or disarm always releases it.")
                                            : qsTr("Press generates a short one-shot action when PRESS is emitted.")
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                        }

                        Item { Layout.fillHeight: true }

                        VcButton {
                            Layout.fillWidth: true
                            theme: root.theme
                            text: qsTr("Save mapping")
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
                            text: qsTr("Remove mapping")
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

    Dialog {
        id: duplicateDialog
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: 380
        modal: true
        title: qsTr("Duplicate profile")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: newProfileName.text = root.uiState.activeProfile + " copy"
        onAccepted: root.uiState.duplicateActiveProfile(newProfileName.text)
        background: Rectangle {
            color: root.theme.surfaceRaised
            radius: root.theme.controlRadius
            border.color: root.theme.borderStrong
        }
        contentItem: ColumnLayout {
            spacing: 12
            Text {
                Layout.fillWidth: true
                text: qsTr("New profile name")
                color: root.theme.textPrimary
                font.family: root.theme.fontFamily
                font.pixelSize: 12
            }
            VcTextField {
                id: newProfileName
                theme: root.theme
                Layout.fillWidth: true
                maximumLength: 64
                selectByMouse: true
            }
        }
    }

    Dialog {
        id: deleteDialog
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: 380
        modal: true
        title: qsTr("Delete profile")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: root.uiState.deleteProfile(root.uiState.activeProfile)
        background: Rectangle {
            color: root.theme.surfaceRaised
            radius: root.theme.controlRadius
            border.color: root.theme.borderStrong
        }
        contentItem: Text {
            text: qsTr("Permanently delete custom profile %1? This cannot be undone.")
                .arg(root.uiState.activeProfile)
            color: root.theme.textPrimary
            font.family: root.theme.fontFamily
            font.pixelSize: 12
            wrapMode: Text.WordWrap
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
