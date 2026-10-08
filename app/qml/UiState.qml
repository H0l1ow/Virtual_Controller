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

    readonly property bool outputAvailable:
        runtimeBackend.outputAvailable
    readonly property bool gestureRecognitionAvailable:
        runtimeBackend.gestureRecognitionAvailable
    readonly property bool cursorControlAvailable:
        runtimeBackend.cursorControlAvailable

    readonly property bool outputArmed:
        runtimeBackend.outputArmed
    readonly property string outputError:
        runtimeBackend.outputError

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

    readonly property string activeProfile:
        runtimeBackend.activeProfile
    readonly property var profileNames:
        runtimeBackend.mappingProfileNames
    readonly property string mappingError:
        runtimeBackend.mappingError
    readonly property string mappingProfileDirectory:
        runtimeBackend.mappingProfileDirectory
    property string outputMode: "Mouse / Keyboard"

    // M2 continuous-control settings are owned by the C++ runtime so the UI
    // cannot drift away from the values actually used by cursor processing.
    readonly property int sensitivity:
        runtimeBackend.sensitivity
    readonly property bool smoothing:
        runtimeBackend.smoothing
    readonly property int cursorSpeedPercent:
        runtimeBackend.cursorSpeedPercent
    readonly property bool invertX:
        runtimeBackend.invertX
    readonly property bool invertY:
        runtimeBackend.invertY
    readonly property int deadzone:
        runtimeBackend.deadzone

    // Backend is the single source of truth for live tracking settings.
    readonly property int detectionConfidence:
        runtimeBackend.detectionConfidence
    readonly property int trackingConfidence:
        runtimeBackend.trackingConfidence
    readonly property bool swapHandedness:
        runtimeBackend.swapHandedness

    readonly property int recognitionThreshold:
        runtimeBackend.recognitionThreshold
    readonly property int debounceMs:
        runtimeBackend.debounceMs
    readonly property int cooldownMs:
        runtimeBackend.cooldownMs
    readonly property bool requireRelease:
        runtimeBackend.requireRelease

    property bool showLandmarks: true
    property bool mirrorPreview: true

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

    readonly property string leftGesture:
        runtimeBackend.leftGesture
    readonly property string rightGesture:
        runtimeBackend.rightGesture
    readonly property real leftGestureConfidence:
        runtimeBackend.leftGestureConfidence
    readonly property real rightGestureConfidence:
        runtimeBackend.rightGestureConfidence
    readonly property string leftGestureEvent:
        runtimeBackend.leftGestureEvent
    readonly property string rightGestureEvent:
        runtimeBackend.rightGestureEvent
    readonly property string gestureBackendName:
        runtimeBackend.gestureBackendName
    readonly property string gestureModelStatus:
        runtimeBackend.gestureModelStatus
    readonly property bool gestureModelActive:
        runtimeBackend.gestureModelActive
    readonly property int cursorX:
        runtimeBackend.cursorX
    readonly property int cursorY:
        runtimeBackend.cursorY
    readonly property bool cursorFrozen:
        runtimeBackend.cursorFrozen

    // Backend-neutral logical action state used by the M4.5 Test page. These
    // values update even when Windows System output is disarmed.
    readonly property bool logicalMouseLeft:
        runtimeBackend.logicalMouseLeft
    readonly property bool logicalMouseRight:
        runtimeBackend.logicalMouseRight
    readonly property real logicalWheel:
        runtimeBackend.logicalWheel
    readonly property var logicalKeys:
        runtimeBackend.logicalKeys

    readonly property int cursorScreenWidth:
        runtimeBackend.cursorScreenWidth
    readonly property int cursorScreenHeight:
        runtimeBackend.cursorScreenHeight

    readonly property var gestureHints:
        catalog.gestureHints
    readonly property var mappingRows:
        runtimeBackend.mappingRows
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

    function setOutputArmed(value) {
        runtimeBackend.outputArmed = value
    }

    function setSensitivity(value) {
        runtimeBackend.sensitivity = Math.round(value)
    }

    function setSmoothing(value) {
        runtimeBackend.smoothing = value
    }

    function setCursorSpeedPercent(value) {
        runtimeBackend.cursorSpeedPercent = Math.round(value)
    }

    function setInvertX(value) {
        runtimeBackend.invertX = value
    }

    function setInvertY(value) {
        runtimeBackend.invertY = value
    }

    function setDeadzone(value) {
        runtimeBackend.deadzone = Math.round(value)
    }

    function setRecognitionThreshold(value) {
        runtimeBackend.recognitionThreshold = Math.round(value)
    }

    function setDebounceMs(value) {
        runtimeBackend.debounceMs = Math.round(value)
    }

    function setCooldownMs(value) {
        runtimeBackend.cooldownMs = Math.round(value)
    }

    function setRequireRelease(value) {
        runtimeBackend.requireRelease = value
    }

    function setActiveProfile(name) {
        runtimeBackend.activeProfile = name
    }

    function addMapping() {
        return runtimeBackend.addMapping()
    }

    function removeMapping(index) {
        runtimeBackend.removeMapping(index)
    }

    function updateMapping(index, hand, gesture, action, behavior, enabled) {
        return runtimeBackend.updateMapping(index, hand, gesture, action, behavior, enabled)
    }

    function saveActiveProfile() {
        return runtimeBackend.saveActiveProfile()
    }

    function reloadActiveProfile() {
        return runtimeBackend.reloadActiveProfile()
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

        setSensitivity(70)
        setCursorSpeedPercent(250)
        setSmoothing(true)
        setInvertX(false)
        setInvertY(false)
        setDeadzone(12)

        setDetectionConfidence(60)
        setTrackingConfidence(55)

        setRecognitionThreshold(80)
        setDebounceMs(120)
        setCooldownMs(250)
        setRequireRelease(true)

        showLandmarks = true
        mirrorPreview = true
        setSwapHandedness(false)

        setActiveProfile("Default")
        setOutputArmed(false)
    }
}
