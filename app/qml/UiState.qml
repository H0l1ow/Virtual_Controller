import QtQuick
import VirtualController.Preferences 1.0
import "catalog"

QtObject {
    id: root

    required property QtObject runtimeBackend

    property QtObject catalog: UiCatalog {}

    // Runtime changes are applied live. Save commits a versioned snapshot to
    // QSettings; unsafe values such as outputArmed are deliberately excluded.
    property bool settingsReady: false
    property string savedSettingsSnapshot: ""
    readonly property bool hasUnsavedSettings:
        settingsReady && JSON.stringify(settingsSnapshot()) !== savedSettingsSnapshot
    readonly property string settingsSaveError: AppPreferences.saveError


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
    readonly property bool autoRecoverCamera: runtimeBackend.autoRecoverCamera
    readonly property string cameraRecoveryStatus: runtimeBackend.cameraRecoveryStatus

    readonly property string activeProfile:
        runtimeBackend.activeProfile
    readonly property var profileNames:
        runtimeBackend.mappingProfileNames
    readonly property string mappingError:
        runtimeBackend.mappingError
    readonly property var mappingIssues: runtimeBackend.mappingIssues
    readonly property bool canDeleteActiveProfile: runtimeBackend.canDeleteActiveProfile
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
    readonly property var gestureThresholds: runtimeBackend.gestureThresholds
    readonly property var gestureThresholdOverrides: runtimeBackend.gestureThresholdOverrides
    readonly property bool calibrationActive: runtimeBackend.calibrationActive
    readonly property string calibrationStatus: runtimeBackend.calibrationStatus
    readonly property int calibrationProgress: runtimeBackend.calibrationProgress
    readonly property string calibrationGesture: runtimeBackend.calibrationGesture
    readonly property string calibrationHand: runtimeBackend.calibrationHand
    readonly property var calibrationResults: runtimeBackend.calibrationResults
    readonly property bool rangeCalibrationActive: runtimeBackend.rangeCalibrationActive
    readonly property int rangeCalibrationProgress: runtimeBackend.rangeCalibrationProgress
    readonly property string rangeCalibrationStatus: runtimeBackend.rangeCalibrationStatus
    readonly property var rangeCalibrationResults: runtimeBackend.rangeCalibrationResults
    readonly property var handCalibration: runtimeBackend.handCalibration
    readonly property bool cursorPlaygroundActive: runtimeBackend.cursorPlaygroundActive
    readonly property var cursorPlaygroundState: runtimeBackend.cursorPlaygroundState
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
    readonly property int cameraIntervalP50Ms: runtimeBackend.cameraIntervalP50Ms
    readonly property int mediaPipeP50Ms: runtimeBackend.mediaPipeP50Ms
    readonly property int gestureP50Ms: runtimeBackend.gestureP50Ms
    readonly property int gestureP95Ms: runtimeBackend.gestureP95Ms
    readonly property int outputDispatchP50Ms: runtimeBackend.outputDispatchP50Ms
    readonly property int outputDispatchP95Ms: runtimeBackend.outputDispatchP95Ms
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
    readonly property var gestureDiagnostics:
        runtimeBackend.gestureDiagnostics
    readonly property int gestureGraceMs:
        runtimeBackend.gestureGraceMs
    readonly property int gestureReleaseThreshold:
        runtimeBackend.gestureReleaseThreshold
    readonly property bool recordingActive:
        runtimeBackend.recordingActive
    readonly property int recordedFrames:
        runtimeBackend.recordedFrames
    readonly property string recordingPath:
        runtimeBackend.recordingPath
    readonly property string recordingError:
        runtimeBackend.recordingError
    readonly property var recordingQuality: runtimeBackend.recordingQuality
    readonly property string recordingsDirectory: runtimeBackend.recordingsDirectory
    readonly property var recordings: runtimeBackend.recordings
    readonly property string recordingsError: runtimeBackend.recordingsError
    readonly property bool replayActive: runtimeBackend.replayActive
    readonly property int replayFrames: runtimeBackend.replayFrames
    readonly property string replayStatus: runtimeBackend.replayStatus
    readonly property string replayError: runtimeBackend.replayError
    readonly property string replayLeftGesture: runtimeBackend.replayLeftGesture
    readonly property string replayRightGesture: runtimeBackend.replayRightGesture
    readonly property string replayLeftLabel: runtimeBackend.replayLeftLabel
    readonly property string replayRightLabel: runtimeBackend.replayRightLabel
    readonly property int cursorX:
        runtimeBackend.cursorX
    readonly property int cursorY:
        runtimeBackend.cursorY
    readonly property bool cursorFrozen:
        runtimeBackend.cursorFrozen
    readonly property bool cursorMovementLocked:
        runtimeBackend.cursorMovementLocked

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

    function attachPiPVideoSink(sink) {
        runtimeBackend.attachPiPVideoSink(sink)
    }

    function setPiPActive(active) {
        runtimeBackend.setPiPActive(active)
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

    function setAutoRecoverCamera(value) {
        runtimeBackend.autoRecoverCamera = value
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

    function setGestureThreshold(gesture, value) {
        runtimeBackend.setGestureThreshold(gesture, Math.round(value))
    }
    function resetGestureThresholds() { runtimeBackend.resetGestureThresholds() }
    function startGestureCalibration(hand, gesture) {
        return runtimeBackend.startGestureCalibration(hand, gesture)
    }
    function cancelGestureCalibration() { runtimeBackend.cancelGestureCalibration() }
    function applyGestureCalibration(gesture) {
        return runtimeBackend.applyGestureCalibration(gesture)
    }
    function clearGestureCalibration() { runtimeBackend.clearGestureCalibration() }
    function startRangeCalibration(side, kind) { return runtimeBackend.startRangeCalibration(side, kind) }
    function cancelRangeCalibration() { runtimeBackend.cancelRangeCalibration() }
    function applyRangeCalibration(side, kind) { return runtimeBackend.applyRangeCalibration(side, kind) }
    function resetRangeCalibration() { runtimeBackend.resetRangeCalibration() }
    function startCursorPlayground() { return runtimeBackend.startCursorPlayground() }
    function stopCursorPlayground() { runtimeBackend.stopCursorPlayground() }

    function setDebounceMs(value) {
        runtimeBackend.debounceMs = Math.round(value)
    }

    function setCooldownMs(value) {
        runtimeBackend.cooldownMs = Math.round(value)
    }

    function setRequireRelease(value) {
        runtimeBackend.requireRelease = value
    }

    function setGestureGraceMs(value) {
        runtimeBackend.gestureGraceMs = value
    }

    function setGestureReleaseThreshold(value) {
        runtimeBackend.gestureReleaseThreshold = value
    }

    function startGestureRecording(leftLabel, rightLabel) {
        return runtimeBackend.startGestureRecording(leftLabel, rightLabel)
    }

    function stopGestureRecording() {
        runtimeBackend.stopGestureRecording()
    }
    function attachRecordingVideoSink(sink) { runtimeBackend.attachRecordingVideoSink(sink) }
    function openRecordingsFolder() { runtimeBackend.openRecordingsFolder() }
    function setRecordingsDirectory(folder) { return runtimeBackend.setRecordingsDirectory(folder) }
    function resetRecordingsDirectory() { runtimeBackend.resetRecordingsDirectory() }
    function refreshRecordings() { runtimeBackend.refreshRecordings() }
    function openRecordingFile(name) { return runtimeBackend.openRecordingFile(name) }
    function deleteRecordingFile(name) { return runtimeBackend.deleteRecordingFile(name) }
    function replayRecordingFile(name) { return runtimeBackend.replayRecordingFile(name) }
    function startGestureReplay(path) { return runtimeBackend.startGestureReplay(path) }
    function stopGestureReplay() { runtimeBackend.stopGestureReplay() }
    function reloadGestureModel() { return runtimeBackend.reloadGestureModel() }

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

    function duplicateActiveProfile(name) {
        return runtimeBackend.duplicateActiveProfile(name)
    }

    function deleteProfile(name) {
        return runtimeBackend.deleteProfile(name)
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

    function settingsSnapshot() {
        return {
            themeMode: AppPreferences.themeMode,
            shortcutTracking: AppPreferences.shortcutTracking,
            shortcutStop: AppPreferences.shortcutStop,
            shortcutNextProfile: AppPreferences.shortcutNextProfile,
            pipEnabled: AppPreferences.pipEnabled,
            pipSizeMode: AppPreferences.pipSizeMode,
            pipShowLandmarks: AppPreferences.pipShowLandmarks,
            pipScreenName: AppPreferences.pipScreenName,
            pipX: AppPreferences.pipX,
            pipY: AppPreferences.pipY,
            autoRecoverCamera: autoRecoverCamera,
            cameraName: cameraNames[cameraIndex] || "",
            formatName: formatNames[formatIndex] || "",
            sensitivity: sensitivity,
            cursorSpeedPercent: cursorSpeedPercent,
            smoothing: smoothing,
            invertX: invertX,
            invertY: invertY,
            deadzone: deadzone,
            detectionConfidence: detectionConfidence,
            trackingConfidence: trackingConfidence,
            swapHandedness: swapHandedness,
            recognitionThreshold: recognitionThreshold,
            gestureThresholdOverrides: gestureThresholdOverrides,
            handCalibration: handCalibration,
            debounceMs: debounceMs,
            cooldownMs: cooldownMs,
            requireRelease: requireRelease,
            gestureGraceMs: gestureGraceMs,
            gestureReleaseThreshold: gestureReleaseThreshold,
            showLandmarks: showLandmarks,
            mirrorPreview: mirrorPreview,
            activeProfile: activeProfile
        }
    }

    function restoreSavedSettings() {
        const saved = AppPreferences.loadSettings()
        const has = (name) => Object.prototype.hasOwnProperty.call(saved, name)

        // Apply to the real C++ runtime, not only to visual controls.
        if (has("sensitivity")) setSensitivity(saved.sensitivity)
        if (has("cursorSpeedPercent")) setCursorSpeedPercent(saved.cursorSpeedPercent)
        if (has("smoothing")) setSmoothing(saved.smoothing)
        if (has("invertX")) setInvertX(saved.invertX)
        if (has("invertY")) setInvertY(saved.invertY)
        if (has("deadzone")) setDeadzone(saved.deadzone)
        if (has("detectionConfidence")) setDetectionConfidence(saved.detectionConfidence)
        if (has("trackingConfidence")) setTrackingConfidence(saved.trackingConfidence)
        if (has("swapHandedness")) setSwapHandedness(saved.swapHandedness)
        if (has("recognitionThreshold")) setRecognitionThreshold(saved.recognitionThreshold)
        if (has("handCalibration")) runtimeBackend.setHandCalibration(saved.handCalibration)
        if (has("gestureThresholdOverrides")) {
            const overrides = saved.gestureThresholdOverrides
            for (const klass of ["FIST", "OPEN_HAND", "POINT", "PINCH"]) {
                if (overrides && Object.prototype.hasOwnProperty.call(overrides, klass))
                    setGestureThreshold(klass, overrides[klass])
            }
        }
        if (has("debounceMs")) setDebounceMs(saved.debounceMs)
        if (has("cooldownMs")) setCooldownMs(saved.cooldownMs)
        if (has("requireRelease")) setRequireRelease(saved.requireRelease)
        if (has("gestureGraceMs")) setGestureGraceMs(saved.gestureGraceMs)
        if (has("gestureReleaseThreshold")) setGestureReleaseThreshold(saved.gestureReleaseThreshold)
        if (has("showLandmarks")) showLandmarks = saved.showLandmarks
        if (has("mirrorPreview")) mirrorPreview = saved.mirrorPreview
        if (has("autoRecoverCamera")) setAutoRecoverCamera(saved.autoRecoverCamera)

        // Camera order may change across reboots. Match labels, never indices.
        if (has("cameraName")) {
            const camera = cameraNames.indexOf(saved.cameraName)
            if (camera >= 0) selectCamera(camera)
        }
        if (has("formatName")) {
            const format = formatNames.indexOf(saved.formatName)
            if (format >= 0) selectFormat(format)
        }
        if (has("activeProfile") && profileNames.indexOf(saved.activeProfile) >= 0)
            setActiveProfile(saved.activeProfile)

        // Do not restore outputArmed or running sessions across launches.
        setOutputArmed(false)
        savedSettingsSnapshot = JSON.stringify(settingsSnapshot())
        settingsReady = true
    }

    function saveSettings() {
        if (!settingsReady)
            return false
        const snapshot = settingsSnapshot()
        if (!AppPreferences.saveSettings(snapshot))
            return false
        savedSettingsSnapshot = JSON.stringify(snapshot)
        return true
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
        resetGestureThresholds()
        resetRangeCalibration()
        setDebounceMs(120)
        setCooldownMs(250)
        setRequireRelease(true)
        setGestureGraceMs(100)
        setGestureReleaseThreshold(55)
        showLandmarks = true
        mirrorPreview = true
        setSwapHandedness(false)
        setActiveProfile("Default")
        setOutputArmed(false)
        AppPreferences.themeMode = "dark"
        AppPreferences.shortcutTracking = "Ctrl+Alt+F9"
        AppPreferences.shortcutStop = "Ctrl+Alt+F10"
        AppPreferences.shortcutNextProfile = "Ctrl+Alt+F11"
        AppPreferences.pipEnabled = true
        AppPreferences.pipSizeMode = "medium"
        AppPreferences.pipShowLandmarks = true
        AppPreferences.pipScreenName = ""
        AppPreferences.setPipPosition(-1, -1)
        setAutoRecoverCamera(true)
        // Reset is a preview until the user clicks Save.
    }

    Component.onCompleted: restoreSavedSettings()
}
