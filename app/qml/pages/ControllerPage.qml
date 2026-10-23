import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    property bool previewActive: true
    property bool immersiveMode: false
    signal immersiveModeRequested(bool enabled)
    signal navigateRequested(int index)

    property bool uiOptionsOpen: false
    property bool showCameraStatus: true
    property bool showGestureHints: false
    property bool showMetrics: true
    property bool showHandBadges: true
    property bool showTrackingPanel: false
    property bool showControlsPanel: false
    property bool showSettingsPanel: false

    // Session-local events only, never a fabricated usage history.
    ListModel { id: recentEvents }
    function addEvent(kind, details) {
        if (!details || details === "NONE" || details === "HOLD") return
        recentEvents.insert(0, {time: Qt.formatTime(new Date(), "hh:mm:ss"),
                                    kind: kind, details: details})
        if (recentEvents.count > 16) recentEvents.remove(recentEvents.count - 1)
    }
    Connections {
        target: root.uiState
        function onLeftGestureEventChanged() {
            if (root.uiState.leftGestureEvent === "PRESS")
                root.addEvent("left", root.uiState.leftGesture)
        }
        function onRightGestureEventChanged() {
            if (root.uiState.rightGestureEvent === "PRESS")
                root.addEvent("right", root.uiState.rightGesture)
        }
        function onOutputArmedChanged() {
            root.addEvent("output", root.uiState.outputArmed ? "ARMED" : "DISARMED")
        }
    }
    onPreviewActiveChanged: {
        if (!previewActive) { advancedDrawer.close(); eventsDrawer.close(); gestureDrawer.close() }
    }
    function setVisibilityOption(key, visible) {
        switch(key) {
        case "cameraStatus": showCameraStatus = visible; break
        case "gestureHints": showGestureHints = visible; break
        case "metrics": showMetrics = visible; break
        case "handBadges": showHandBadges = visible; break
        case "trackingPanel": showTrackingPanel = visible; break
        case "controlsPanel": showControlsPanel = visible; break
        case "settingsPanel": showSettingsPanel = visible; break
        }
    }
    function showAllFullscreenUi() {
        showCameraStatus = true
        showGestureHints = true
        showMetrics = true
        showHandBadges = true
        showTrackingPanel = true
        showControlsPanel = true
        showSettingsPanel = true
    }
    onImmersiveModeChanged: {
        if (immersiveMode) {
            showTrackingPanel = false
            showControlsPanel = false
            showSettingsPanel = false
        }
        uiOptionsOpen = false
        advancedDrawer.close()
        gestureDrawer.close()
    }

    // Dashboard is a single viewport: no vertical or nested scrollbars.
    // The image gets all height not needed by the compact action row.
    ColumnLayout {
        id: dashboardLayout
        visible: !root.immersiveMode
        anchors.fill: parent
        anchors.margins: Math.max(9, root.theme.pageMargin)
        spacing: 9

        CameraStage {
            id: dashboardCamera
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            theme: root.theme
            uiState: root.uiState
            previewActive: root.previewActive && !root.immersiveMode
            immersiveMode: false
            dashboardStatusOverlay: true
            showGestureHints: false
            showHandBadges: false
            showCameraStatus: false
            showMetrics: width > 790
            onFullscreenRequested: function(enabled) { root.immersiveModeRequested(enabled) }
        }

        GridLayout {
            id: actions
            Layout.fillWidth: true
            Layout.preferredHeight: columns === 4 ? 42 : 92
            columns: width >= 690 ? 4 : 2
            columnSpacing: 8
            rowSpacing: 8

            VcButton {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                theme: root.theme
                text: root.uiState.canStop ? qsTr("Stop tracking") : qsTr("Start tracking")
                iconName: root.uiState.canStop ? "stop" : "play"
                variant: root.uiState.canStop ? "secondary" : "primary"
                enabled: root.uiState.canStart || root.uiState.canStop
                onClicked: root.uiState.togglePipeline()
            }
            VcButton {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                theme: root.theme
                iconName: "hand"
                text: qsTr("Gestures & calibration")
                onClicked: gestureDrawer.open()
            }
            VcButton {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                theme: root.theme
                iconName: "settings"
                text: qsTr("Advanced options")
                onClicked: advancedDrawer.open()
            }
            VcButton {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                theme: root.theme
                iconName: "info"
                text: qsTr("Recent events")
                onClicked: eventsDrawer.open()
            }
        }
    }

    DashboardGestureDrawer {
        id: gestureDrawer
        parent: Overlay.overlay
        width: Math.min(418, root.width * 0.85)
        height: parent ? parent.height : root.height
        theme: root.theme
        uiState: root.uiState
        onFullSettingsRequested: root.navigateRequested(2)
    }

    // Session history is still available without consuming permanent camera
    // space or introducing scrolling in the dashboard itself.
    Drawer {
        id: eventsDrawer
        parent: Overlay.overlay
        edge: Qt.RightEdge
        width: Math.min(370, root.width * 0.82)
        height: parent ? parent.height : root.height
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: root.theme.backgroundAlt
            border.width: 1
            border.color: root.theme.borderStrong
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Recent events")
                    color: root.theme.textPrimary
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }
                VcButton {
                    theme: root.theme
                    text: qsTr("Clear")
                    variant: "ghost"
                    enabled: recentEvents.count > 0
                    onClicked: recentEvents.clear()
                }
                VcIconButton {
                    theme: root.theme
                    iconName: "stop"
                    onClicked: eventsDrawer.close()
                }
            }
            Text {
                visible: recentEvents.count === 0
                Layout.fillWidth: true
                text: qsTr("No events in this session yet. Start tracking to see recognized gestures.")
                wrapMode: Text.WordWrap
                color: root.theme.textSecondary
                font.pixelSize: 12
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: recentEvents
                clip: true
                spacing: 4
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 40
                    radius: 7
                    color: index % 2 ? "transparent" : root.theme.surfaceRaised
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        Text {
                            text: time
                            color: root.theme.textMuted
                            font.pixelSize: 11
                        }
                        Text {
                            Layout.fillWidth: true
                            text: kind === "output" ? qsTr("System output") + " / " + details
                                : (kind === "left" ? qsTr("Left hand") : qsTr("Right hand"))
                                  + " / " + qsTr(details)
                            color: root.theme.textPrimary
                            elide: Text.ElideRight
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }

    Drawer {
        id: advancedDrawer
        parent: Overlay.overlay
        edge: Qt.RightEdge
        width: Math.min(388, root.width * 0.82)
        height: parent ? parent.height : root.height
        modal: true
        interactive: !root.immersiveMode
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: root.theme.backgroundAlt
            border.width: 1
            border.color: root.theme.borderStrong
        }
        contentItem: Loader {
            active: advancedDrawer.visible
            sourceComponent: Component {
                ScrollView {
                    id: drawerScroll
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ColumnLayout {
                        x: 12
                        y: 12
                        width: Math.max(1, drawerScroll.availableWidth - 24)
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 53
                            spacing: 10
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Advanced options")
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }
                            VcButton {
                                theme: root.theme
                                text: qsTr("Close")
                                iconName: "stop"
                                onClicked: advancedDrawer.close()
                            }
                        }
                        QuickSettingsCard {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 246
                            theme: root.theme
                            uiState: root.uiState
                        }
                        CursorTrackingCard {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 246
                            theme: root.theme
                            uiState: root.uiState
                        }
                        Item { Layout.preferredHeight: 12 }
                    }
                }
            }
        }
    }

    // Fullscreen remains a camera-only presentation mode. Exactly one
    // display sink is attached at a time; tracking is unaffected.
    CameraStage {
        id: fullscreenStage
        visible: root.immersiveMode
        anchors.fill: parent
        theme: root.theme
        uiState: root.uiState
        previewActive: root.previewActive && root.immersiveMode
        immersiveMode: true
        uiOptionsOpen: root.uiOptionsOpen
        showCameraStatus: root.showCameraStatus
        showGestureHints: root.showGestureHints
        dashboardStatusOverlay: true
        showMetrics: root.showMetrics
        showHandBadges: root.showHandBadges
        overlayBottomInset: fullscreenCards.visible ? fullscreenCards.height + 22 : 0
        onFullscreenRequested: function(enabled) { root.immersiveModeRequested(enabled) }
        onUiOptionsRequested: root.uiOptionsOpen = !root.uiOptionsOpen
    }
    RowLayout {
        id: fullscreenCards
        visible: root.immersiveMode && (root.showTrackingPanel || root.showControlsPanel || root.showSettingsPanel)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        height: 244
        spacing: 10
        CursorTrackingCard {
            visible: root.showTrackingPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: root.theme
            uiState: root.uiState
            overlayMode: true
        }
        ControlsCard {
            visible: root.showControlsPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: root.theme
            uiState: root.uiState
            overlayMode: true
        }
        QuickSettingsCard {
            visible: root.showSettingsPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: root.theme
            uiState: root.uiState
            overlayMode: true
        }
    }
    ViewOptionsPanel {
        visible: root.immersiveMode && root.uiOptionsOpen
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 14
        anchors.topMargin: 66
        theme: root.theme
        showCameraStatus: root.showCameraStatus
        showGestureHints: root.showGestureHints
        showMetrics: root.showMetrics
        showHandBadges: root.showHandBadges
        showTrackingPanel: root.showTrackingPanel
        showControlsPanel: root.showControlsPanel
        showSettingsPanel: root.showSettingsPanel
        onOptionRequested: function(key, visible) { root.setVisibilityOption(key, visible) }
        onShowAllRequested: root.showAllFullscreenUi()
    }
}
