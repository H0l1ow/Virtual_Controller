import QtQuick
import "mock"

QtObject {
    id: root

    required property QtObject runtimeBackend

    // Reuse the UI catalog prepared in the mock. Camera and tracking values
    // below come from the real C++ runtime in M1.
    property QtObject catalog: UiMock {
    }

    readonly property bool cameraRunning:
        runtimeBackend.cameraRunning

    readonly property bool pipelineRunning:
        runtimeBackend.pipelineRunning

    readonly property bool outputAvailable: false
    readonly property bool gestureRecognitionAvailable: false
    readonly property bool cursorControlAvailable: false

    property bool outputArmed: false

    readonly property var cameraNames:
        runtimeBackend.cameraNames

    readonly property var formatNames:
        runtimeBackend.formatNames

    readonly property int cameraIndex:
        runtimeBackend.cameraIndex

    readonly property int formatIndex:
        runtimeBackend.formatIndex

    readonly property string errorMessage:
        runtimeBackend.errorMessage

    property string activeProfile: "Default"

    readonly property var profileNames:
        catalog.profileNames

    property string outputMode:
        "Mouse / Keyboard"

    // Retained for the UI; cursor control is implemented in a later milestone.
    property int sensitivity: 70
    property bool smoothing: true
    property int deadzone: 12

    // These two already configure MediaPipe when the M1 pipeline starts.
    property int detectionConfidence:
        runtimeBackend.detectionConfidence

    property int trackingConfidence:
        runtimeBackend.trackingConfidence

    // Gesture settings stay visible but are inactive in M1.
    property int recognitionThreshold: 80
    property int debounceMs: 120
    property int cooldownMs: 250
    property bool requireRelease: true

    property bool showLandmarks: true
    property bool mirrorPreview: false

    property bool swapHandedness:
        runtimeBackend.swapHandedness

    onDetectionConfidenceChanged: {
        runtimeBackend.detectionConfidence = detectionConfidence
    }

    onTrackingConfidenceChanged: {
        runtimeBackend.trackingConfidence = trackingConfidence
    }

    onSwapHandednessChanged: {
        runtimeBackend.swapHandedness = swapHandedness
    }

    readonly property bool leftTracked:
        runtimeBackend.leftTracked

    readonly property bool rightTracked:
        runtimeBackend.rightTracked

    readonly property real leftConfidence:
        runtimeBackend.leftConfidence

    readonly property real rightConfidence:
        runtimeBackend.rightConfidence

    readonly property var leftLandmarks:
        runtimeBackend.leftLandmarks

    readonly property var rightLandmarks:
        runtimeBackend.rightLandmarks

    readonly property int fps:
        runtimeBackend.fps

    readonly property string resolution:
        runtimeBackend.resolution

    readonly property int latencyMs:
        runtimeBackend.latencyMs

    readonly property string trackingStatus:
        runtimeBackend.trackingStatus

    // GestureRecognizer does not exist in M1.
    readonly property string leftGesture: "NONE"
    readonly property string rightGesture: "NONE"

    // Cursor mapping does not exist in M1.
    readonly property int cursorX: 0
    readonly property int cursorY: 0

    readonly property var gestureHints:
        catalog.gestureHints

    readonly property var mappingRows:
        catalog.mappingRows

    readonly property var gestureLibrary:
        catalog.gestureLibrary

    // Gamepad is outside M1 and therefore always neutral.
    readonly property string leftStickValue:
        "0.00 / 0.00"

    readonly property string rightStickValue:
        "0.00 / 0.00"

    readonly property string triggerValue:
        "0.00 / 0.00"

    readonly property var gamepadInputs: [
        { keyName: "A", value: "Released" },
        { keyName: "B", value: "Released" },
        { keyName: "X", value: "Released" },
        { keyName: "Y", value: "Released" },
        { keyName: "LB / RB", value: "Off / Off" },
        { keyName: "D-Pad", value: "Neutral" }
    ]

    function attachVideoOutput(output) {
        runtimeBackend.attachVideoOutput(output)
    }

    function selectCamera(index) {
        runtimeBackend.selectCamera(index)
    }

    function selectFormat(index) {
        runtimeBackend.selectFormat(index)
    }

    function setPipelineRunning(enabled) {
        if (enabled)
            runtimeBackend.start()
        else
            runtimeBackend.stop()
    }

    function togglePipeline() {
        runtimeBackend.togglePipeline()
    }

    function resetSettingsDefaults() {
        if (pipelineRunning)
            return

        sensitivity = 70
        smoothing = true
        deadzone = 12

        detectionConfidence = 60
        trackingConfidence = 55

        recognitionThreshold = 80
        debounceMs = 120
        cooldownMs = 250
        requireRelease = true

        showLandmarks = true
        mirrorPreview = false
        swapHandedness = false

        activeProfile = "Default"
        outputArmed = false
    }
}
