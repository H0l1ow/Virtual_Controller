import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

Item {
    id: root
    required property QtObject theme
    required property QtObject uiState
    signal backRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12
        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: qsTr("Performance Monitor")
            subtitle: qsTr("Timing diagnostics of the running camera and gesture pipeline.")
            VcButton {
                theme: root.theme
                text: qsTr("Back to gesture tests")
                iconName: "reset"
                onClicked: root.backRequested()
            }
        }
        VcCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            theme: root.theme
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14
                Text {
                    text: qsTr("Performance Monitor (median / P95)")
                    color: root.theme.textPrimary
                    font.family: root.theme.fontFamily
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }
                Repeater {
                    model: [
                        { title: qsTr("Camera frame interval"), p50: root.uiState.cameraIntervalP50Ms, p95: -1 },
                        { title: qsTr("Frame conversion"), p50: root.uiState.conversionP50Ms, p95: root.uiState.conversionP95Ms },
                        { title: qsTr("MediaPipe tracking"), p50: root.uiState.mediaPipeP50Ms, p95: root.uiState.inferenceP95Ms },
                        { title: qsTr("Gesture classification"), p50: root.uiState.gestureP50Ms, p95: root.uiState.gestureP95Ms },
                        { title: qsTr("Output dispatch"), p50: root.uiState.outputDispatchP50Ms, p95: root.uiState.outputDispatchP95Ms },
                        { title: qsTr("End-to-end pipeline"), p50: root.uiState.latencyP50Ms, p95: root.uiState.latencyP95Ms }
                    ]
                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 12
                        Text {
                            Layout.fillWidth: true
                            text: modelData.title
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                        }
                        Text {
                            text: modelData.p50 + " ms" + (modelData.p95 < 0 ? "" : " / " + modelData.p95 + " ms")
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Camera: inter-frame period; Output: submission time, not physical Windows input latency.")
                    wrapMode: Text.WordWrap
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pixelSize: 11
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
