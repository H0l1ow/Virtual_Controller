import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property int currentIndex: 0
    property bool collapsed: false
    property bool toolsExpanded: false
    signal pageSelected(int index)
    signal collapseToggled()

    implicitWidth: collapsed ? 67 : 218
    Behavior on implicitWidth { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
    color: theme.topBar
    border.width: 1
    border.color: theme.borderSubtle
    clip: true

    // Only five primary destinations. Existing Test and Gamepad screens
    // remain accessible in Tools without pretending that they are ready
    // as primary, general-purpose controls.
    readonly property var destinations: [
        { page: 0, title: qsTr("Dashboard"), icon: "monitor" },
        { page: 2, title: qsTr("Gestures"), icon: "hand" },
        { page: 1, title: qsTr("Mapping"), icon: "mapping" },
        { page: 5, title: qsTr("ML"), icon: "ml" },
        { page: 6, title: qsTr("Settings"), icon: "settings" }
    ]

    function openPage(page) {
        if (page === 3 || page === 4)
            toolsExpanded = true
        pageSelected(page)
    }
    onCurrentIndexChanged: {
        if (currentIndex === 3 || currentIndex === 4)
            toolsExpanded = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: 17
        anchors.bottomMargin: 12
        spacing: 5

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            spacing: 9
            HandLogo {
                Layout.preferredWidth: 37
                Layout.preferredHeight: 37
                Layout.leftMargin: root.collapsed ? 3 : 5
                color: root.theme.accent
            }
            ColumnLayout {
                visible: !root.collapsed
                Layout.fillWidth: true
                spacing: 1
                Text {
                    text: "VirtualController"
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 14
                    font.weight: Font.Bold
                }
                Text {
                    text: qsTr("Gesture control")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 7
            Layout.bottomMargin: 11
            color: root.theme.borderSubtle
        }

        Repeater {
            model: root.destinations
            delegate: Rectangle {
                id: nav
                required property var modelData
                readonly property bool selected: root.currentIndex === modelData.page
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: root.theme.controlRadius
                color: selected ? root.theme.accentSoft
                    : navMouse.containsMouse ? root.theme.surfaceHover : "transparent"
                border.width: selected ? 1 : 0
                border.color: selected ? root.theme.accentDim : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
                Rectangle {
                    visible: nav.selected
                    width: 3
                    height: 28
                    radius: 2
                    color: root.theme.accent
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.collapsed ? 14 : 14
                    anchors.rightMargin: 9
                    spacing: 12
                    VcIcon {
                        name: nav.modelData.icon
                        Layout.preferredWidth: 23
                        Layout.preferredHeight: 23
                        color: nav.selected ? root.theme.accent : root.theme.textSecondary
                    }
                    Text {
                        visible: !root.collapsed
                        Layout.fillWidth: true
                        text: nav.modelData.title
                        font.family: root.theme.fontFamily
                        font.pixelSize: 14
                        font.weight: nav.selected ? Font.DemiBold : Font.Normal
                        color: nav.selected ? root.theme.textPrimary : root.theme.textSecondary
                        elide: Text.ElideRight
                    }
                }
                MouseArea {
                    id: navMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.openPage(nav.modelData.page)
                }
                ToolTip.visible: root.collapsed && navMouse.containsMouse
                ToolTip.text: nav.modelData.title
                ToolTip.delay: 500
            }
        }

        // Actual, implemented auxiliary screens; not dead-end shortcuts.
        Rectangle {
            id: toolHeader
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            radius: root.theme.controlRadius
            color: toolMouse.containsMouse ? root.theme.surfaceHover : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 11
                VcIcon {
                    name: "target"
                    color: root.currentIndex === 3 || root.currentIndex === 4
                        ? root.theme.accent : root.theme.textMuted
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                }
                Text {
                    visible: !root.collapsed
                    Layout.fillWidth: true
                    text: qsTr("Tools")
                    font.family: root.theme.fontFamily
                    color: root.theme.textSecondary
                    font.pixelSize: 12
                }
                Text {
                    visible: !root.collapsed
                    text: root.toolsExpanded ? "\u2303" : "\u2304"
                    font.pixelSize: 17
                    color: root.theme.textMuted
                }
            }
            MouseArea {
                id: toolMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.toolsExpanded = !root.toolsExpanded
            }
            ToolTip.visible: root.collapsed && toolMouse.containsMouse
            ToolTip.text: qsTr("Tools")
            ToolTip.delay: 500
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.toolsExpanded
            spacing: 2
            Repeater {
                model: [
                    { page: 3, title: qsTr("Test lab"), icon: "target" },
                    { page: 4, title: qsTr("Gamepad monitor"), icon: "gamepad" }
                ]
                delegate: Rectangle {
                    id: toolNav
                    required property var modelData
                    readonly property bool selected: root.currentIndex === modelData.page
                    Layout.fillWidth: true
                    Layout.preferredHeight: 39
                    radius: root.theme.controlRadius
                    color: selected ? root.theme.accentSoft : toolNavMouse.containsMouse
                        ? root.theme.surfaceHover : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: root.collapsed ? 14 : 24
                        spacing: 10
                        VcIcon {
                            name: toolNav.modelData.icon
                            color: toolNav.selected ? root.theme.accent : root.theme.textMuted
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                        }
                        Text {
                            visible: !root.collapsed
                            Layout.fillWidth: true
                            text: toolNav.modelData.title
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        id: toolNavMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openPage(toolNav.modelData.page)
                    }
                    ToolTip.visible: root.collapsed && toolNavMouse.containsMouse
                    ToolTip.text: toolNav.modelData.title
                    ToolTip.delay: 500
                }
            }
        }
        Item { Layout.fillHeight: true }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.collapsed ? 38 : 72
            radius: root.theme.controlRadius
            color: root.theme.surfaceRaised
            border.width: 1
            border.color: root.theme.borderSubtle
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 9
                Rectangle {
                    Layout.preferredWidth: 9
                    Layout.preferredHeight: 9
                    radius: 5
                    color: root.uiState.outputArmed
                        ? (root.uiState.outputActive ? root.theme.success : root.theme.warning)
                        : root.uiState.pipelineRunning ? root.theme.success : root.theme.neutral
                }
                ColumnLayout {
                    visible: !root.collapsed
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: root.uiState.outputArmed
                            ? (root.uiState.outputActive ? qsTr("Control active") : qsTr("Control paused"))
                            : root.uiState.pipelineRunning ? qsTr("Tracking active") : qsTr("Ready")
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: root.uiState.pipelineRunning ? qsTr("Camera connected") : qsTr("Camera stopped")
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            radius: root.theme.controlRadius
            color: collapseMouse.containsMouse ? root.theme.surfaceHover : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 13
                spacing: 10
                Text {
                    text: root.collapsed ? ">>" : "<<"
                    color: root.theme.textSecondary
                    font.pixelSize: 14
                    Layout.preferredWidth: 22
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    visible: !root.collapsed
                    text: qsTr("Collapse menu")
                    color: root.theme.textSecondary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                }
            }
            MouseArea {
                id: collapseMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.collapseToggled()
            }
        }
    }
}
