import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState

    // Controlled by Main.qml. Normal mode is intentionally the unchanged M1
    // layout. Fullscreen turns the camera stage into the page background and
    // overlays the existing Controller cards on top of it.
    property bool immersiveMode: false
    signal immersiveModeRequested(bool enabled)

    property bool uiOptionsOpen: false

    // Fullscreen starts in a camera-focused layout. Diagnostic overlays stay
    // visible, while the three large lower cards start hidden. The user may
    // enable any card from Fullscreen UI for the current fullscreen session.
    property bool showCameraStatus: true
    property bool showGestureHints: true
    property bool showMetrics: true
    property bool showHandBadges: true
    property bool showTrackingPanel: false
    property bool showControlsPanel: false
    property bool showSettingsPanel: false

    readonly property real normalLowerPanelHeight: Math.max(
        235,
        Math.min(
            270,
            root.height - (2 * root.theme.pageMargin) - 12 - 390
        )
    )

    readonly property bool anyBottomPanelVisible:
        root.showTrackingPanel
        || root.showControlsPanel
        || root.showSettingsPanel

    function setVisibilityOption(key, visible) {
        switch (key) {
        case "cameraStatus":
            root.showCameraStatus = visible
            break
        case "gestureHints":
            root.showGestureHints = visible
            break
        case "metrics":
            root.showMetrics = visible
            break
        case "handBadges":
            root.showHandBadges = visible
            break
        case "trackingPanel":
            root.showTrackingPanel = visible
            break
        case "controlsPanel":
            root.showControlsPanel = visible
            break
        case "settingsPanel":
            root.showSettingsPanel = visible
            break
        }
    }

    function showAllFullscreenUi() {
        root.showCameraStatus = true
        root.showGestureHints = true
        root.showMetrics = true
        root.showHandBadges = true
        root.showTrackingPanel = true
        root.showControlsPanel = true
        root.showSettingsPanel = true
    }

    function resetFullscreenUiDefaults() {
        root.showCameraStatus = true
        root.showGestureHints = true
        root.showMetrics = true
        root.showHandBadges = true
        root.showTrackingPanel = false
        root.showControlsPanel = false
        root.showSettingsPanel = false
        root.uiOptionsOpen = false
    }

    onImmersiveModeChanged: {
        if (root.immersiveMode)
            root.resetFullscreenUiDefaults()
        else
            root.uiOptionsOpen = false
    }

    CameraStage {
        id: cameraStage
        z: 0

        anchors.fill: parent
        anchors.leftMargin: root.immersiveMode ? 0 : root.theme.pageMargin
        anchors.rightMargin: root.immersiveMode ? 0 : root.theme.pageMargin
        anchors.topMargin: root.immersiveMode ? 0 : root.theme.pageMargin
        anchors.bottomMargin: root.immersiveMode
            ? 0
            : root.theme.pageMargin + lowerPanel.height + 12

        theme: root.theme
        uiState: root.uiState
        immersiveMode: root.immersiveMode
        uiOptionsOpen: root.uiOptionsOpen

        // Visibility switches affect fullscreen only. Returning to normal mode
        // always reproduces the original M1 Controller page.
        showCameraStatus: !root.immersiveMode || root.showCameraStatus
        showGestureHints: !root.immersiveMode || root.showGestureHints
        showMetrics: !root.immersiveMode || root.showMetrics
        showHandBadges: !root.immersiveMode || root.showHandBadges

        overlayBottomInset: root.immersiveMode && lowerPanel.visible
            ? lowerPanel.height + 22
            : 0

        onFullscreenRequested: function(enabled) {
            root.immersiveModeRequested(enabled)
        }

        onUiOptionsRequested: {
            root.uiOptionsOpen = !root.uiOptionsOpen
        }
    }

    RowLayout {
        id: lowerPanel
        z: 2

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.immersiveMode ? 18 : root.theme.pageMargin
        anchors.rightMargin: root.immersiveMode ? 18 : root.theme.pageMargin
        anchors.bottomMargin: root.immersiveMode ? 18 : root.theme.pageMargin

        height: root.normalLowerPanelHeight
        spacing: 12
        visible: !root.immersiveMode || root.anyBottomPanelVisible

        CursorTrackingCard {
            visible: !root.immersiveMode || root.showTrackingPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1.18
            theme: root.theme
            uiState: root.uiState
            overlayMode: root.immersiveMode
        }

        ControlsCard {
            visible: !root.immersiveMode || root.showControlsPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 0.92
            theme: root.theme
            uiState: root.uiState
            overlayMode: root.immersiveMode
        }

        QuickSettingsCard {
            visible: !root.immersiveMode || root.showSettingsPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 0.98
            theme: root.theme
            uiState: root.uiState
            overlayMode: root.immersiveMode
        }
    }

    ViewOptionsPanel {
        z: 5
        visible: root.immersiveMode && root.uiOptionsOpen

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 14
        anchors.topMargin: 66

        theme: root.theme
        showCameraStatus: root.showCameraStatus
        showGestureHints: root.showGestureHints
        showMetrics: root.showMetrics
        showHandBadges: root.showHandBadges
        showTrackingPanel: root.showTrackingPanel
        showControlsPanel: root.showControlsPanel
        showSettingsPanel: root.showSettingsPanel

        onOptionRequested: function(key, visible) {
            root.setVisibilityOption(key, visible)
        }

        onShowAllRequested: root.showAllFullscreenUi()
    }
}
