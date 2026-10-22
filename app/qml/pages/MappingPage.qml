import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VirtualController.I18n 1.0
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int selectedRow: uiState.mappingRows.length > 0 ? 0 : -1
    property string gestureFilter: "ALL"
    readonly property var selectedMapping: selectedRow >= 0 && selectedRow < uiState.mappingRows.length
        ? uiState.mappingRows[selectedRow] : ({})
    readonly property var handOptions: ["Left", "Right"]
    readonly property var gestureOptions: ["PINCH", "FIST", "POINT", "OPEN HAND"]
    readonly property var behaviorOptions: ["Press", "Hold", "Toggle"]
    readonly property var actionOptions: [
        "Left click", "Right click", "Scroll up", "Scroll down", "Freeze cursor",
        "Enable cursor movement", "Key Space", "Key Enter", "Key Escape", "Key Tab",
        "Key Left", "Key Right", "Key Up", "Key Down", "Key Ctrl", "Key Shift", "Key Alt"
    ]
    readonly property var filterOptions: [
        {code: "ALL", title: qsTr("All gestures"), icon: "hand"},
        {code: "OPEN HAND", title: qsTr("Open Hand"), icon: "open"},
        {code: "POINT", title: qsTr("Point"), icon: "point"},
        {code: "FIST", title: qsTr("Fist"), icon: "fist"},
        {code: "PINCH", title: qsTr("Pinch"), icon: "pinch"}
    ]
    readonly property var displayedRows: {
        const rows = []
        const all = uiState.mappingRows || []
        for (let i = 0; i < all.length; ++i) {
            if (gestureFilter === "ALL" || all[i].source === gestureFilter)
                rows.push({mapping: all[i], realIndex: i})
        }
        return rows
    }
    function optionIndex(options, value, fallback) {
        const at = options.indexOf(value)
        return at >= 0 ? at : fallback
    }
    function displayOption(text) {
        LanguageManager.language
        return qsTr(text || "")
    }
    function issueDescription(issue) {
        LanguageManager.language
        switch (issue.code) {
        case "duplicate_id": return qsTr("Two mappings have the same ID.")
        case "scroll_toggle": return qsTr("Scroll cannot use Toggle.")
        case "cursor_press": return qsTr("Cursor movement requires Hold or Toggle.")
        case "duplicate_action": return qsTr("The same action is assigned twice.")
        case "conflicting_behavior": return qsTr("Conflicting mapping behaviors.")
        case "cursor_gate_freeze": return qsTr("Cursor gating conflicts with freezing.")
        case "drag_exclusive": return qsTr("Cursor movement and drag cannot require different gestures on the same hand.")
        case "no_cursor_gate": return qsTr("No gesture unlocks cursor movement.")
        default: return qsTr("Potential mapping conflict.")
        }
    }
    function syncEditor() {
        if (selectedRow < 0 || selectedRow >= uiState.mappingRows.length) return
        const m = uiState.mappingRows[selectedRow]
        handCombo.currentIndex = optionIndex(handOptions, m.hand || "Right", 1)
        gestureCombo.currentIndex = optionIndex(gestureOptions, m.source || "PINCH", 0)
        actionCombo.currentIndex = optionIndex(actionOptions, m.action || "Left click", 0)
        behaviorCombo.currentIndex = optionIndex(behaviorOptions, m.behavior || "Hold", 1)
        enabledSwitch.checked = m.enabled === true
    }
    function openEditor(index) {
        selectedRow = index
        Qt.callLater(syncEditor)
        editPopup.open()
    }
    onSelectedRowChanged: Qt.callLater(syncEditor)
    Component.onCompleted: Qt.callLater(syncEditor)

    Popup {
        id: mappingActions
        x: Math.max(8, root.width - width - 18)
        y: 68
        width: 228
        padding: 10
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            radius: root.theme.controlRadius
            color: root.theme.surface
            border.color: root.theme.borderStrong
        }
        contentItem: ColumnLayout {
            spacing: 7
            VcButton {
                Layout.fillWidth: true
                theme: root.theme
                text: qsTr("Reload profile")
                iconName: "reset"
                onClicked: { mappingActions.close(); root.uiState.reloadActiveProfile() }
            }
            VcButton {
                Layout.fillWidth: true
                theme: root.theme
                text: qsTr("Delete profile")
                iconName: "trash"
                variant: "danger"
                enabled: root.uiState.canDeleteActiveProfile
                onClicked: { mappingActions.close(); deleteDialog.open() }
            }
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: Math.max(1, scroll.availableWidth)
            spacing: 13
            Item { Layout.preferredHeight: 4 }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                spacing: 9
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: qsTr("Mapping")
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: root.theme.fontHero
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Assign hand gestures to mouse and keyboard actions")
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: root.theme.fontBody
                        elide: Text.ElideRight
                    }
                }
            }
            Flow {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                spacing: 9
                VcComboBox {
                    id: mappingProfileCombo
                    theme: root.theme
                    implicitWidth: 205
                    model: root.uiState.profileNames
                    currentIndex: Math.max(0, root.uiState.profileNames.indexOf(root.uiState.activeProfile))
                    onActivated: root.uiState.setActiveProfile(currentText)
                }
                VcButton {
                    theme: root.theme
                    iconName: "plus"
                    text: qsTr("Add mapping")
                    variant: "primary"
                    onClicked: {
                        const i = root.uiState.addMapping()
                        if (i >= 0) root.openEditor(i)
                    }
                }
                VcButton {
                    theme: root.theme
                    iconName: "save"
                    text: qsTr("Save profile")
                    onClicked: root.uiState.saveActiveProfile()
                }
                VcButton {
                    theme: root.theme
                    iconName: "plus"
                    text: qsTr("Duplicate profile")
                    onClicked: duplicateDialog.open()
                }
                VcButton {
                    theme: root.theme
                    iconName: "settings"
                    text: qsTr("More")
                    onClicked: mappingActions.open()
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                visible: root.uiState.mappingError.length > 0
                text: root.displayOption(root.uiState.mappingError)
                color: root.theme.error
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                columns: root.width >= 900 ? 2 : 1
                columnSpacing: 13
                rowSpacing: 13
                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 370
                    Layout.preferredHeight: 490
                    theme: root.theme
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        Text {
                            text: qsTr("Gestures")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Select a gesture to filter its assigned actions.")
                            color: root.theme.textMuted
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                        Repeater {
                            model: root.filterOptions
                            delegate: Rectangle {
                                id: filterCard
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 67
                                radius: root.theme.controlRadius
                                color: root.gestureFilter === modelData.code
                                    ? root.theme.accentSoft : root.theme.surfaceRaised
                                border.width: 1
                                border.color: root.gestureFilter === modelData.code
                                    ? root.theme.accentDim : root.theme.borderSubtle
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 11
                                    spacing: 10
                                    GestureGlyph {
                                        gesture: filterCard.modelData.code === "ALL" ? "open" : filterCard.modelData.icon
                                        color: root.theme.accent
                                        Layout.preferredWidth: 35
                                        Layout.preferredHeight: 35
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: filterCard.modelData.title
                                        color: root.theme.textPrimary
                                        font.family: root.theme.fontFamily
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        text: root.gestureFilter === filterCard.modelData.code ? "\u2713" : ">"
                                        color: root.theme.accent
                                        font.pixelSize: 16
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.gestureFilter = filterCard.modelData.code
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
                VcCard {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: 680
                    Layout.preferredHeight: 490
                    theme: root.theme
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Assigned actions")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }
                            StatusBadge {
                                theme: root.theme
                                text: root.uiState.mappingIssues.length === 0 ? qsTr("No conflicts")
                                    : qsTr("Issues: %1").arg(root.uiState.mappingIssues.length)
                                kind: root.uiState.mappingIssues.length === 0 ? "success" : "warning"
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: root.displayedRows.length === 0
                            text: qsTr("No mappings for this gesture yet. Click Add mapping.")
                            color: root.theme.textMuted
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: root.displayedRows
                            clip: true
                            spacing: 7
                            delegate: Rectangle {
                                id: mapCard
                                required property var modelData
                                width: ListView.view.width
                                height: 72
                                radius: root.theme.controlRadius
                                color: root.selectedRow === modelData.realIndex
                                    ? root.theme.accentSoft : root.theme.surfaceRaised
                                border.width: 1
                                border.color: root.selectedRow === modelData.realIndex
                                    ? root.theme.accentDim : root.theme.borderSubtle
                                opacity: modelData.mapping.enabled ? 1 : 0.65
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 11
                                    spacing: 9
                                    VcIcon {
                                        name: "hand"
                                        Layout.preferredWidth: 23
                                        Layout.preferredHeight: 23
                                        color: root.theme.accent
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text {
                                            Layout.fillWidth: true
                                            text: root.displayOption(mapCard.modelData.mapping.source) + "  /  "
                                                + root.displayOption(mapCard.modelData.mapping.hand)
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                            color: root.theme.textPrimary
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: root.displayOption(mapCard.modelData.mapping.action) + "  -  "
                                                + root.displayOption(mapCard.modelData.mapping.behavior)
                                            font.pixelSize: 11
                                            color: root.theme.textSecondary
                                            elide: Text.ElideRight
                                        }
                                    }
                                    StatusBadge {
                                        theme: root.theme
                                        text: mapCard.modelData.mapping.enabled ? qsTr("Enabled") : qsTr("Disabled")
                                        kind: mapCard.modelData.mapping.enabled ? "success" : "neutral"
                                    }
                                    VcIcon {
                                        name: "settings"
                                        Layout.preferredWidth: 18
                                        Layout.preferredHeight: 18
                                        color: root.theme.textMuted
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.openEditor(mapCard.modelData.realIndex)
                                }
                            }
                        }
                    }
                }
            }
            DisclosurePanel {
                Layout.fillWidth: true
                Layout.leftMargin: root.theme.pageMargin
                Layout.rightMargin: root.theme.pageMargin
                theme: root.theme
                title: qsTr("Mapping validation")
                description: qsTr("Review conflicts before arming system input")
                iconName: "info"
                bodyHeight: 210
                bodyComponent: Component {
                    ScrollView {
                        clip: true
                        ColumnLayout {
                            width: parent.width
                            spacing: 8
                            Text {
                                Layout.fillWidth: true
                                text: root.uiState.mappingIssues.length === 0
                                    ? qsTr("No mapping conflicts detected.")
                                    : qsTr("Mapping issues found: %1").arg(root.uiState.mappingIssues.length)
                                color: root.theme.textPrimary
                                font.pixelSize: 12
                            }
                            Repeater {
                                model: root.uiState.mappingIssues
                                delegate: Text {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    text: root.issueDescription(modelData)
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 11
                                    color: modelData.severity === "error" ? root.theme.error : root.theme.warning
                                }
                            }
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: root.theme.pageMargin }
        }
    }

    Popup {
        id: editPopup
        parent: Overlay.overlay
        x: Math.max(8, parent.width - width - 15)
        y: Math.max(12, (parent.height - height) / 2)
        width: Math.min(415, parent.width - 24)
        height: Math.min(670, parent.height - 24)
        padding: 0
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: Qt.callLater(root.syncEditor)
        background: Rectangle {
            color: root.theme.surface
            border.color: root.theme.borderStrong
            radius: root.theme.cardRadius
        }
        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                Layout.leftMargin: 16
                Layout.rightMargin: 13
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Edit mapping")
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
                VcIconButton {
                    theme: root.theme
                    iconName: "stop"
                    onClicked: editPopup.close()
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 10
                    anchors.margins: 16
                    Item { Layout.preferredHeight: 8 }
                    Text { text: qsTr("Hand"); color: root.theme.textSecondary; font.pixelSize: 11 }
                    VcComboBox { id: handCombo; Layout.fillWidth: true; theme: root.theme; model: root.handOptions; labelFormatter: root.displayOption }
                    Text { text: qsTr("Gesture"); color: root.theme.textSecondary; font.pixelSize: 11 }
                    VcComboBox { id: gestureCombo; Layout.fillWidth: true; theme: root.theme; model: root.gestureOptions; labelFormatter: root.displayOption }
                    Text { text: qsTr("Action"); color: root.theme.textSecondary; font.pixelSize: 11 }
                    VcComboBox { id: actionCombo; Layout.fillWidth: true; theme: root.theme; model: root.actionOptions; labelFormatter: root.displayOption }
                    Text { text: qsTr("Behavior"); color: root.theme.textSecondary; font.pixelSize: 11 }
                    VcComboBox { id: behaviorCombo; Layout.fillWidth: true; theme: root.theme; model: root.behaviorOptions; labelFormatter: root.displayOption }
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Enabled")
                            color: root.theme.textPrimary
                            font.pixelSize: 12
                        }
                        VcSwitch { id: enabledSwitch; theme: root.theme; checked: false }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Mapping changes take effect after Save mapping.")
                        color: root.theme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: 11
                    }
                    Item { Layout.preferredHeight: 8 }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                Layout.leftMargin: 11
                Layout.rightMargin: 11
                spacing: 8
                VcButton {
                    theme: root.theme
                    text: qsTr("Remove")
                    iconName: "trash"
                    variant: "danger"
                    enabled: root.selectedRow >= 0
                    onClicked: {
                        const index = root.selectedRow
                        editPopup.close()
                        root.uiState.removeMapping(index)
                        root.selectedRow = Math.min(index, root.uiState.mappingRows.length - 1)
                    }
                }
                Item { Layout.fillWidth: true }
                VcButton {
                    theme: root.theme
                    text: qsTr("Save mapping")
                    iconName: "save"
                    variant: "primary"
                    enabled: root.selectedRow >= 0
                    onClicked: {
                        root.uiState.updateMapping(root.selectedRow, handCombo.currentText,
                            gestureCombo.currentText, actionCombo.currentText,
                            behaviorCombo.currentText, enabledSwitch.checked)
                        editPopup.close()
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
