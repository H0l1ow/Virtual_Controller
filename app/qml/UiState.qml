import QtQuick
import "catalog"

QtObject {
    id: root

    required property QtObject runtimeBackend

    property QtObject catalog: UiCatalog {}

    readonly property string runtimeStateName:
        runtimeBackend.runtimeStateName
    readonly property bool canStart:
        runtimeBackend.canStart
    readonly property bool canStop:
        runtimeBackend.canStop

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
    property string outputMode: "Mouse / Keyboard"

    // Future control settings retained for the UI only.
    property int sensitivity: 70
    property bool smoothing: true
    property int deadzone: 12

    // Backend is the single source of truth for live tracking settings.
    readonly property int detectionConfidence:
        runtimeBackend.detectionConfidence
    readonly property int trackingConfidence:
        runtimeBackend.trackingConfidence
    readonly property bool swapHandedness:
        runtimeBackend.swapHandedness

    property int recognitionThreshold: 80
    property int debounceMs: 120
    property int cooldownMs: 250
    property bool requireRelease: true

    property bool showLandmarks: true
    property bool mirrorPreview: false

    readonly property bool leftTracked:
        runtimeBackend.leftTracked
    readonly property bool rightTracked:
        runtimeBackend.rightTracked

    // These are MediaPipe handedness classification scores, not generic
    // tracking confidence values.
    readonly property real leftConfidence:
        runtimeBackend.leftConfidence
    readonly property real rightConfidence:
        runtimeBackend.rightConfidence
    readonly property string leftReportedSide:
        runtimeBackend.leftReportedSide
    readonly property string rightReportedSide:
        runtimeBackend.rightReportedSide

    readonly property var leftLandmarks:
        runtimeBackend.leftLandmarks
    readonly property var rightLandmarks:
        runtimeBackend.rightLandmarks

    // Metrics. fps/latencyMs remain aliases for existing components.
    readonly property int fps:
        runtimeBackend.processedFps
    readonly property int cameraFps:
        runtimeBackend.cameraFps
    readonly property int processedFps:
        runtimeBackend.processedFps
    readonly property int latencyMs:
        runtimeBackend.latencyMs
    readonly property int latencyP50Ms:
        runtimeBackend.latencyP50Ms
    readonly property int latencyP95Ms:
        runtimeBackend.latencyP95Ms
    readonly property int inferenceP50Ms:
        runtimeBackend.inferenceP50Ms
    readonly property int inferenceP95Ms:
        runtimeBackend.inferenceP95Ms
    readonly property int conversionP50Ms:
        runtimeBackend.conversionP50Ms
    readonly property int conversionP95Ms:
        runtimeBackend.conversionP95Ms
    readonly property real replacedFrames:
        Number(runtimeBackend.replacedFrames)
    readonly property real replacedPercent:
        runtimeBackend.replacedPercent

    readonly property string resolution:
        runtimeBackend.resolution
    readonly property string trackingStatus:
        runtimeBackend.trackingStatus

    readonly property string leftGesture: "NONE"
    readonly property string rightGesture: "NONE"
    readonly property int cursorX: 0
    readonly property int cursorY: 0

    readonly property var gestureHints:
        catalog.gestureHints
    readonly property var mappingRows:
        catalog.mappingRows
    readonly property var gestureLibrary:
        catalog.gestureLibrary

    readonly property string leftStickValue: "0.00 / 0.00"
    readonly property string rightStickValue: "0.00 / 0.00"
    readonly property string triggerValue: "0.00 / 0.00"
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

    function setDetectionConfidence(value) {
        runtimeBackend.detectionConfidence = Math.round(value)
    }

    function setTrackingConfidence(value) {
        runtimeBackend.trackingConfidence = Math.round(value)
    }

    function setSwapHandedness(value) {
        runtimeBackend.swapHandedness = value
    }

    function setPipelineRunning(enabled) {
        if (enabled && canStart)
            runtimeBackend.start()
        else if (!enabled && canStop)
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

        setDetectionConfidence(60)
        setTrackingConfidence(55)

        recognitionThreshold = 80
        debounceMs = 120
        cooldownMs = 250
        requireRelease = true

        showLandmarks = true
        mirrorPreview = false
        setSwapHandedness(false)

        activeProfile = "Default"
        outputArmed = false
    }
}
