import QtQuick

QtObject {
    id: root

    // This object is the mock implementation of the UI state contract. Runtime
    // services can replace it later without forcing the QML pages to know where
    // camera, tracking, ML or output data comes from.
    property bool cameraRunning: true
    property bool pipelineRunning: true
    property bool outputArmed: false
    property bool hasUnsavedSettings: false
    property string settingsSaveError: ""

    readonly property string runtimeStateName:
        pipelineRunning ? "Running" : "Stopped"
    readonly property bool canStart: !pipelineRunning
    readonly property bool canStop: pipelineRunning

    readonly property bool outputAvailable: true
    readonly property bool gestureRecognitionAvailable: true
    readonly property bool cursorControlAvailable: true
    readonly property string errorMessage: ""
    property bool autoRecoverCamera: true
    readonly property string cameraRecoveryStatus: ""
    readonly property string outputError: ""

    property var cameraNames: [
        "Integrated Camera",
        "USB Camera",
        "Virtual Camera"
    ]

    property var formatNames: [
        "640 x 480 @ 30 FPS | YUYV",
        "1280 x 720 @ 30 FPS | NV12",
        "1280 x 720 @ 60 FPS | MJPEG"
    ]

    property int cameraIndex: 0
    property int formatIndex: 0

    readonly property var leftLandmarks: []
    readonly property var rightLandmarks: []

    property string activeProfile: "Default"
    property var profileNames: ["Default", "Desktop", "Presentation", "Game"]
    property string mappingError: ""
    readonly property bool canDeleteActiveProfile: ["Default", "Desktop", "Presentation", "Game"].indexOf(activeProfile) === -1
    readonly property var mappingIssues: [{code: "no_cursor_gate", severity: "warning", first: -1, second: -1}]
    readonly property string mappingProfileDirectory: "<mock>/profiles"
    property string outputMode: "Mouse / Keyboard"

    property int sensitivity: 70
    property bool smoothing: true
    property int cursorSpeedPercent: 250
    property bool invertX: false
    property bool invertY: false
    property int deadzone: 12

    property int detectionConfidence: 60
    property int trackingConfidence: 55
    property int recognitionThreshold: 80
    property var gestureThresholdOverrides: ({"FIST": -1, "OPEN_HAND": -1, "POINT": -1, "PINCH": -1})
    readonly property var gestureThresholds: ({
        "FIST": gestureThresholdOverrides.FIST < 0 ? recognitionThreshold : gestureThresholdOverrides.FIST,
        "OPEN_HAND": gestureThresholdOverrides.OPEN_HAND < 0 ? recognitionThreshold : gestureThresholdOverrides.OPEN_HAND,
        "POINT": gestureThresholdOverrides.POINT < 0 ? recognitionThreshold : gestureThresholdOverrides.POINT,
        "PINCH": gestureThresholdOverrides.PINCH < 0 ? recognitionThreshold : gestureThresholdOverrides.PINCH
    })
    property bool calibrationActive: false
    property string calibrationStatus: "IDLE"
    property int calibrationProgress: 0
    property string calibrationGesture: "PINCH"
    property string calibrationHand: "Right"
    property var calibrationResults: []
    property bool rangeCalibrationActive: false
    property int rangeCalibrationProgress: 0
    property string rangeCalibrationStatus: "IDLE"
    property var rangeCalibrationResults: [
        {side: "Left", motionValid: false, closedValid: false, openValid: false,
         spanX: 0, spanY: 0, closedDistance: 0, openDistance: 0},
        {side: "Right", motionValid: false, closedValid: false, openValid: false,
         spanX: 0, spanY: 0, closedDistance: 0, openDistance: 0}
    ]
    property var handCalibration: ({Left: {motionX: 1, motionY: 1, pinchLow: 0.17, pinchHigh: 0.43},
                                    Right: {motionX: 1, motionY: 1, pinchLow: 0.17, pinchHigh: 0.43}})
    property bool cursorPlaygroundActive: false
    property var cursorPlaygroundState: ({
        active: false, complete: false, tracked: rightTracked,
        x: 480, y: 270, targetX: 190, targetY: 125,
        targetIndex: 0, targetCount: 6, dwellPercent: 0,
        averageMs: 0, averageErrorPx: 0, jitterPx: 0, missedClicks: 0
    })
    property int debounceMs: 120
    property int cooldownMs: 250
    property bool requireRelease: true
    property bool showLandmarks: true
    property bool mirrorPreview: true
    property bool swapHandedness: false

    // Mock availability can be toggled independently to exercise one-hand-loss
    // scenarios without stopping the whole pipeline.
    property bool leftHandAvailable: true
    property bool rightHandAvailable: true
    readonly property bool physicalLeftTracked: pipelineRunning && leftHandAvailable
    readonly property bool physicalRightTracked: pipelineRunning && rightHandAvailable
    readonly property bool leftTracked: swapHandedness
        ? physicalRightTracked
        : physicalLeftTracked
    readonly property bool rightTracked: swapHandedness
        ? physicalLeftTracked
        : physicalRightTracked

    property int fps: cameraRunning ? 30 : 0
    readonly property int cameraFps: cameraRunning ? 60 : 0
    readonly property int processedFps: fps
    readonly property int replacedFrames: pipelineRunning ? 42 : 0
    readonly property real replacedPercent: pipelineRunning ? 48.0 : 0.0
    readonly property int latencyP50Ms: pipelineRunning ? 18 : 0
    readonly property int latencyP95Ms: pipelineRunning ? 31 : 0
    readonly property int inferenceP50Ms: pipelineRunning ? 14 : 0
    readonly property int inferenceP95Ms: pipelineRunning ? 24 : 0
    readonly property int conversionP50Ms: pipelineRunning ? 2 : 0
    readonly property int conversionP95Ms: pipelineRunning ? 4 : 0
    property string resolution: "640 × 480"
    property int latencyMs: pipelineRunning ? 20 : 0
    property int cursorX: pipelineRunning ? 1280 : 0
    property int cursorY: pipelineRunning ? 720 : 0
    property bool cursorFrozen: false
    property bool cursorMovementLocked: false
    property bool logicalMouseLeft: false
    property bool logicalMouseRight: false
    property real logicalWheel: 0.0
    property var logicalKeys: []
    readonly property int cursorScreenWidth: 1920
    readonly property int cursorScreenHeight: 1080
    readonly property string trackingStatus: {
        if (!pipelineRunning)
            return "Stopped"
        if (leftTracked && rightTracked)
            return "Stable"
        if (leftTracked || rightTracked)
            return "Degraded"
        return "No hands"
    }

    readonly property string leftGesture: !leftTracked
        ? "NONE"
        : swapHandedness ? "PINCH" : "OPEN HAND"
    readonly property real leftConfidence: !leftTracked
        ? 0.0
        : swapHandedness ? 0.93 : 0.96
    readonly property string rightGesture: !rightTracked
        ? "NONE"
        : swapHandedness ? "OPEN HAND" : "PINCH"
    readonly property real rightConfidence: !rightTracked
        ? 0.0
        : swapHandedness ? 0.96 : 0.93
    readonly property real leftGestureConfidence: leftTracked ? 0.91 : 0.0
    readonly property real rightGestureConfidence: rightTracked ? 0.94 : 0.0
    readonly property string leftGestureEvent: leftTracked ? "HOLD" : "IDLE"
    readonly property string rightGestureEvent: rightTracked ? "PRESS" : "IDLE"
    readonly property string gestureBackendName: "Rules"
    readonly property string gestureModelStatus: "Mock rule fallback"
    readonly property bool gestureModelActive: false
    property int gestureGraceMs: 100
    property int gestureReleaseThreshold: 55
    readonly property var gestureDiagnostics: [
        { reason: "MOCK", active: leftGesture, candidate: "NONE", inGrace: false,
          inCooldown: false, pinch: 0.2, curl: [0.1, 0.1, 0.1, 0.1],
          scores: [ { name: "FIST", value: 0.05 }, { name: "OPEN_HAND", value: 0.91 },
                    { name: "POINT", value: 0.08 }, { name: "PINCH", value: 0.15 } ] },
        { reason: "MOCK", active: rightGesture, candidate: "NONE", inGrace: false,
          inCooldown: false, pinch: 0.1, curl: [0.2, 0.1, 0.1, 0.1],
          scores: [ { name: "FIST", value: 0.04 }, { name: "OPEN_HAND", value: 0.13 },
                    { name: "POINT", value: 0.1 }, { name: "PINCH", value: 0.94 } ] }
    ]
    property bool recordingActive: false
    property int recordedFrames: 0
    property string recordingPath: ""
    property string recordingError: "Mock mode: no dataset is written"
    readonly property var recordingQuality: ({
        target: 150,
        left: {
            labelled: false,
            usable: 0,
            missing: 0,
            percent: 0,
            trackedPercent: 0
        },
        right: {
            labelled: false,
            usable: 0,
            missing: 0,
            percent: 0,
            trackedPercent: 0
        }
    })
    property string recordingsDirectory: "Mock mode"
    property var recordings: []
    property string recordingsError: ""
    property bool replayActive: false
    property int replayFrames: 0
    property string replayStatus: ""
    property string replayError: ""
    property string replayLeftGesture: "NONE"
    property string replayRightGesture: "NONE"
    property string replayLeftLabel: "UNLABELED"
    property string replayRightLabel: "UNLABELED"
    property int cameraIntervalP50Ms: 33
    property int mediaPipeP50Ms: 8
    property int gestureP50Ms: 1
    property int gestureP95Ms: 3
    property int outputDispatchP50Ms: 1
    property int outputDispatchP95Ms: 2
    readonly property string leftReportedSide: leftTracked ? "L" : "-"
    readonly property string rightReportedSide: rightTracked ? "R" : "-"

    property var gestureHints: [
        {
            id: "open_hand",
            gesture: "open",
            title: qsTr("Open Hand"),
            action: qsTr("Move Cursor"),
            active: true
        },
        {
            id: "pinch",
            gesture: "pinch",
            title: qsTr("Pinch"),
            action: qsTr("Left Click"),
            active: true
        },
        {
            id: "point",
            gesture: "point",
            title: qsTr("Point"),
            action: qsTr("Right Click"),
            active: true
        },
        {
            id: "thumb_up",
            gesture: "up",
            title: qsTr("Thumbs Up"),
            action: qsTr("Scroll Up"),
            active: true
        },
        {
            id: "thumb_down",
            gesture: "down",
            title: qsTr("Thumbs Down"),
            action: qsTr("Scroll Down"),
            active: true
        },
        {
            id: "fist",
            gesture: "fist",
            title: qsTr("Fist"),
            action: qsTr("Pause / Resume"),
            active: true
        }
    ]

    // Mock M4 mapping rows.
    property var mappingRows: [
        { id: "right_pinch_left_mouse", hand: "Right", source: "PINCH", type: "Gesture", action: "Left click", output: "Mouse", behavior: "Hold", state: "Enabled", enabled: true },
        { id: "right_fist_right_mouse", hand: "Right", source: "FIST", type: "Gesture", action: "Right click", output: "Mouse", behavior: "Press", state: "Disabled", enabled: false },
        { id: "left_pinch_space", hand: "Left", source: "PINCH", type: "Gesture", action: qsTr("Key Space"), output: "Keyboard", behavior: "Press", state: "Disabled", enabled: false }
    ]

    property var gestureLibrary: [
        {
            id: "open_hand",
            gesture: "open",
            title: qsTr("Open Hand"),
            category: "Static",
            description: qsTr("Open hand with fingers extended"),
            enabled: true
        },
        {
            id: "pinch",
            gesture: "pinch",
            title: qsTr("Pinch"),
            category: "Static",
            description: qsTr("Thumb and index finger pinch"),
            enabled: true
        },
        {
            id: "point",
            gesture: "point",
            title: qsTr("Point"),
            category: "Static",
            description: qsTr("Index finger extended for pointing"),
            enabled: true
        },
        {
            id: "fist",
            gesture: "fist",
            title: qsTr("Fist"),
            category: "Static",
            description: qsTr("Closed hand"),
            enabled: true
        },
        {
            id: "thumb_up",
            gesture: "up",
            title: qsTr("Thumbs Up"),
            category: "Static",
            description: qsTr("Thumb pointing upward"),
            enabled: true
        },
        {
            id: "thumb_down",
            gesture: "down",
            title: qsTr("Thumbs Down"),
            category: "Static",
            description: qsTr("Thumb pointing downward"),
            enabled: true
        },
        {
            id: "swipe_horizontal",
            gesture: "swipe",
            title: qsTr("Swipe Left / Right"),
            category: "Dynamic",
            description: qsTr("Horizontal hand motion"),
            enabled: false
        },
        {
            id: "swipe_vertical",
            gesture: "swipe",
            title: qsTr("Swipe Up / Down"),
            category: "Dynamic",
            description: qsTr("Vertical hand motion"),
            enabled: false
        },
        {
            id: "two_hand_rotate",
            gesture: "rotate",
            title: qsTr("Two-Hand Rotate"),
            category: "Two-hand",
            description: qsTr("Relative rotation of both hands"),
            enabled: false
        }
    ]

    // Logical controller state remains observable while output is disarmed.
    // Each hand-dependent part returns to neutral independently when tracking
    // for that hand is lost; stopping the pipeline therefore neutralizes all.
    readonly property string leftStickValue: leftTracked ? "-0.32 / 0.84" : "0.00 / 0.00"
    readonly property string rightStickValue: rightTracked ? "0.51 / 0.12" : "0.00 / 0.00"
    readonly property string triggerValue: "0.00 / "
        + (rightTracked ? "0.72" : "0.00")
    readonly property var gamepadInputs: [
        { keyName: "A", value: "Released" },
        { keyName: "B", value: rightTracked ? "Pressed" : "Released" },
        { keyName: "X", value: "Released" },
        { keyName: "Y", value: "Released" },
        { keyName: "LB / RB", value: "Off / Off" },
        { keyName: "D-Pad", value: "Neutral" }
    ]

    function attachPiPVideoSink(sink) {}
    function setPiPActive(active) {}
    function saveSettings() { return true }

    function attachVideoOutput(output) {
        // No-op in mock mode.
    }

    function selectCamera(index) {
        if (index >= 0 && index < cameraNames.length)
            cameraIndex = index
    }

    function selectFormat(index) {
        if (index >= 0 && index < formatNames.length)
            formatIndex = index
    }


    function setDetectionConfidence(value) {
        detectionConfidence = Math.round(value)
    }

    function setTrackingConfidence(value) {
        trackingConfidence = Math.round(value)
    }

    function setSwapHandedness(value) {
        swapHandedness = value
    }

    function setOutputArmed(value) {
        outputArmed = value && pipelineRunning && !cursorPlaygroundActive
    }

    function setSensitivity(value) {
        sensitivity = Math.round(value)
    }

    function setSmoothing(value) {
        smoothing = value
    }

    function setCursorSpeedPercent(value) {
        cursorSpeedPercent = Math.round(value)
    }

    function setInvertX(value) {
        invertX = value
    }

    function setInvertY(value) {
        invertY = value
    }

    function setDeadzone(value) {
        deadzone = Math.round(value)
    }

    function setRecognitionThreshold(value) {
        recognitionThreshold = Math.round(value)
    }
    function setGestureThreshold(gesture, value) {
        const values = Object.assign({}, gestureThresholdOverrides)
        values[gesture] = Math.round(value)
        gestureThresholdOverrides = values
    }
    function resetGestureThresholds() {
        gestureThresholdOverrides = ({"FIST": -1, "OPEN_HAND": -1, "POINT": -1, "PINCH": -1})
    }
    function startGestureCalibration(hand, gesture) {
        calibrationHand = hand
        calibrationGesture = gesture
        calibrationActive = true
        calibrationStatus = "CAPTURING"
        calibrationProgress = 0
        outputArmed = false
        return true
    }
    function cancelGestureCalibration() { calibrationActive = false; calibrationStatus = "CANCELLED" }
    function applyGestureCalibration(gesture) { return false }
    function clearGestureCalibration() { calibrationResults = []; calibrationStatus = "IDLE" }
    function startCursorPlayground() {
        outputArmed = false
        cursorPlaygroundActive = true
        cursorPlaygroundState = Object.assign({}, cursorPlaygroundState, {active: true})
        return true
    }
    function stopCursorPlayground() {
        cursorPlaygroundActive = false
        cursorPlaygroundState = Object.assign({}, cursorPlaygroundState, {active: false})
    }

    function setDebounceMs(value) { debounceMs = Math.round(value) }
    function setCooldownMs(value) { cooldownMs = Math.round(value) }
    function setRequireRelease(value) { requireRelease = value }
    function setGestureGraceMs(value) { gestureGraceMs = value }
    function setGestureReleaseThreshold(value) { gestureReleaseThreshold = value }
    function startRangeCalibration(side, kind) { return false }
    function cancelRangeCalibration() { rangeCalibrationActive = false }
    function applyRangeCalibration(side, kind) { return false }
    function resetRangeCalibration() { rangeCalibrationActive = false }
    function startGestureRecording(leftLabel, rightLabel) { return false }
    function startGestureReplay(path) { replayError = "Mock replay unavailable"; return false }
    function stopGestureReplay() { replayActive = false }
    function reloadGestureModel() { return false }
    function stopGestureRecording() { recordingActive = false }
    function attachRecordingVideoSink(sink) {}
    function openRecordingsFolder() {}
    function setRecordingsDirectory(folder) { return false }
    function resetRecordingsDirectory() {}
    function refreshRecordings() {}
    function openRecordingFile(name) { return false }
    function deleteRecordingFile(name) { return false }
    function replayRecordingFile(name) { return false }
    function setActiveProfile(name) { activeProfile = name }

    function addMapping() {
        const copy = mappingRows.slice(0)
        copy.push({ id: "custom_" + copy.length, hand: "Right", source: "PINCH", type: "Gesture", action: "Left click", output: "Mouse", behavior: "Hold", state: "Disabled", enabled: false })
        mappingRows = copy
        return copy.length - 1
    }

    function removeMapping(index) {
        if (index < 0 || index >= mappingRows.length) return
        const copy = mappingRows.slice(0)
        copy.splice(index, 1)
        mappingRows = copy
    }

    function updateMapping(index, hand, gesture, action, behavior, enabled) {
        if (index < 0 || index >= mappingRows.length) return false
        const copy = mappingRows.slice(0)
        copy[index] = {
            id: copy[index].id, hand: hand, source: gesture, type: "Gesture",
            action: action, output: action.indexOf("Key ") === 0 ? "Keyboard" : "Mouse",
            behavior: behavior, state: enabled ? "Enabled" : "Disabled", enabled: enabled
        }
        mappingRows = copy
        return true
    }

    function saveActiveProfile() { return true }
    function reloadActiveProfile() { return true }
    function duplicateActiveProfile(name) {
        const trimmed = name.trim()
        if (!trimmed || profileNames.some(p => p.toLowerCase() === trimmed.toLowerCase())) {
            mappingError = "Invalid or duplicate profile name."
            return false
        }
        profileNames = profileNames.concat([trimmed])
        activeProfile = trimmed
        mappingError = ""
        outputArmed = false
        return true
    }
    function deleteProfile(name) {
        if (!canDeleteActiveProfile || name !== activeProfile) return false
        profileNames = profileNames.filter(p => p !== name)
        activeProfile = "Default"
        outputArmed = false
        return true
    }

    function setPipelineRunning(enabled) {
        cameraRunning = enabled
        pipelineRunning = enabled
        if (!enabled) {
            outputArmed = false
            stopCursorPlayground()
            cancelGestureCalibration()
        }
    }

    function togglePipeline() {
        setPipelineRunning(!pipelineRunning)
    }

    function resetSettingsDefaults() {
        sensitivity = 70
        cursorSpeedPercent = 250
        smoothing = true
        invertX = false
        invertY = false
        deadzone = 12
        detectionConfidence = 60
        trackingConfidence = 55
        recognitionThreshold = 80
        resetGestureThresholds()
        debounceMs = 120
        cooldownMs = 250
        requireRelease = true
        showLandmarks = true
        mirrorPreview = true
        swapHandedness = false
        activeProfile = "Default"
        outputArmed = false
    }
}
