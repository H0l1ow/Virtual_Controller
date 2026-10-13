import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    signal backRequested()
    readonly property var stats: uiState.cursorPlaygroundState || ({})
    readonly property real planeW: 960
    readonly property real planeH: 540
    readonly property real radius: 35

    onVisibleChanged: {
        if (!visible && uiState.cursorPlaygroundActive)
            uiState.stopCursorPlayground()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Cursor Playground")
            subtitle: qsTr("Measure virtual pointer control with no mouse or keyboard events sent to Windows.")
            VcButton {
                theme: root.theme
                text: qsTr("Back to gesture tests")
                iconName: "reset"
                onClicked: {
                    root.uiState.stopCursorPlayground()
                    root.backRequested()
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            VcCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme
                Item {
                    id: stageArea
                    anchors.fill: parent
                    anchors.margins: 14
                    Rectangle {
                        id: testPlane
                        anchors.centerIn: parent
                        width: Math.min(parent.width, parent.height * 16 / 9)
                        height: width * 9 / 16
                        radius: root.theme.cardRadius
                        color: root.theme.backgroundAlt
                        border.width: 1
                        border.color: root.theme.borderStrong
                        clip: true

                        // The C++ test plane uses logical 960x540 coordinates.
                        readonly property real sx: width / root.planeW
                        readonly property real sy: height / root.planeH
                        Rectangle {
                            id: goal
                            visible: root.uiState.cursorPlaygroundActive
                            width: root.radius * 2 * testPlane.sx
                            height: root.radius * 2 * testPlane.sy
                            x: (Number(root.stats.targetX) || 0) * testPlane.sx - width / 2
                            y: (Number(root.stats.targetY) || 0) * testPlane.sy - height / 2
                            radius: Math.min(width, height) / 2
                            color: "transparent"
                            border.width: 3
                            border.color: root.theme.accent
                            Rectangle {
                                anchors.centerIn: parent
                                width: 7
                                height: 7
                                radius: 4
                                color: root.theme.accent
                            }
                        }
                        Rectangle {
                            visible: root.uiState.cursorPlaygroundActive
                            x: (Number(root.stats.x) || 0) * testPlane.sx - width / 2
                            y: (Number(root.stats.y) || 0) * testPlane.sy - height / 2
                            width: 13
                            height: 13
                            radius: 7
                            color: root.theme.textPrimary
                            border.width: 2
                            border.color: root.theme.background
                        }
                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 40
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            visible: !root.uiState.cursorPlaygroundActive
                            text: qsTr("Press Start exercise to display the virtual pointer and targets.")
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 13
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            width: parent.width * (Number(root.stats.dwellPercent) || 0) / 100
                            height: 5
                            color: root.theme.accent
                            visible: root.uiState.cursorPlaygroundActive
                        }
                    }
                }
            }
            VcCard {
                Layout.preferredWidth: 310
                Layout.fillHeight: true
                theme: root.theme
                ScrollView {
                    id: sidebarScroll
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    contentHeight: sidebarContents.implicitHeight + 24
                    ColumnLayout {
                        id: sidebarContents
                        x: 12
                        y: 12
                        width: Math.max(1, sidebarScroll.availableWidth - 24)
                        spacing: 12
                    Text {
                        text: qsTr("Pointer accuracy")
                        color: root.theme.textPrimary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Move your RIGHT hand to each target. Hold inside the circle for 550 ms, or trigger a mapped left click. No Windows input is generated.")
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                    }
                    StatusBadge {
                        theme: root.theme
                        text: !root.uiState.pipelineRunning ? qsTr("Start camera first")
                            : root.stats.complete ? qsTr("Exercise complete")
                            : root.uiState.cursorPlaygroundActive && root.stats.tracked
                                ? qsTr("Right hand tracked")
                                : root.uiState.cursorPlaygroundActive
                                    ? qsTr("Waiting for right hand") : qsTr("Not running")
                        kind: root.stats.complete ? "success" : "neutral"
                    }
                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: root.uiState.cursorPlaygroundActive
                            ? qsTr("Restart exercise") : qsTr("Start exercise")
                        iconName: "play"
                        enabled: root.uiState.pipelineRunning
                        onClicked: root.uiState.startCursorPlayground()
                    }
                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: qsTr("Stop exercise")
                        iconName: "reset"
                        enabled: root.uiState.cursorPlaygroundActive
                        onClicked: root.uiState.stopCursorPlayground()
                    }
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
                    Repeater {
                        model: [
                            {label: qsTr("Targets"), value: (Number(root.stats.targetIndex) || 0) + " / " + (Number(root.stats.targetCount) || 6)},
                            {label: qsTr("Average acquisition"), value: Math.round(Number(root.stats.averageMs) || 0) + " ms"},
                            {label: qsTr("Average final error"), value: (Number(root.stats.averageErrorPx) || 0).toFixed(1) + " px"},
                            {label: qsTr("Movement jitter"), value: (Number(root.stats.jitterPx) || 0).toFixed(1) + " px"},
                            {label: qsTr("Missed clicks"), value: Number(root.stats.missedClicks) || 0}
                        ]
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: root.theme.textMuted
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                            }
                            Text {
                                text: String(modelData.value)
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Safety: system input is disarmed while this exercise is open. To resume Windows control, leave this mode and arm input manually.")
                        color: root.theme.warning
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                    }
                    Item { Layout.preferredHeight: 5 }
                    }
                }
            }
        }
        VcCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            theme: root.theme
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 9
                anchors.bottomMargin: 9
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 17
                    ColumnLayout {
                        Layout.preferredWidth: 250
                        spacing: 1
                        Text {
                            text: qsTr("Sensitivity") + ": " + root.uiState.sensitivity + "%"
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            color: root.theme.textSecondary
                        }
                        VcSlider {
                            theme: root.theme
                            Layout.fillWidth: true
                            value: root.uiState.sensitivity
                            stepSize: 1
                            onMoved: root.uiState.setSensitivity(value)
                        }
                    }
                    ColumnLayout {
                        Layout.preferredWidth: 250
                        spacing: 1
                        Text {
                            text: qsTr("Deadzone") + ": " + root.uiState.deadzone + "%"
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            color: root.theme.textSecondary
                        }
                        VcSlider {
                            theme: root.theme
                            Layout.fillWidth: true
                            value: root.uiState.deadzone
                            stepSize: 1
                            onMoved: root.uiState.setDeadzone(value)
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: qsTr("Smoothing")
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 11
                    }
                    VcSwitch {
                        theme: root.theme
                        checked: root.uiState.smoothing
                        onToggled: root.uiState.setSmoothing(checked)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Adjust parameters live, then use Settings > Save to keep them after restart.")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 10
                    elide: Text.ElideRight
                }
            }
        }
    }
}
