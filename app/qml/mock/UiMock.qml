import QtQuick

QtObject {
    id: root

    // This object is the mock implementation of the UI state contract. Runtime
    // services can replace it later without forcing the QML pages to know where
    // camera, tracking, ML or output data comes from.
    property bool cameraRunning: true
    property bool pipelineRunning: true
    property bool outputArmed: false

    readonly property string runtimeStateName:
        pipelineRunning ? "Running" : "Stopped"
    readonly property bool canStart: !pipelineRunning
    readonly property bool canStop: pipelineRunning

    readonly property bool outputAvailable: true
    readonly property bool gestureRecognitionAvailable: true
    readonly property bool cursorControlAvailable: true
    readonly property string errorMessage: ""
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
    readonly property string mappingError: ""
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
    readonly property string leftReportedSide: leftTracked ? "L" : "-"
    readonly property string rightReportedSide: rightTracked ? "R" : "-"

    property var gestureHints: [
        {
            id: "open_hand",
            gesture: "open",
            title: "Open Hand",
            action: "Move Cursor",
            active: true
        },
        {
            id: "pinch",
            gesture: "pinch",
            title: "Pinch",
            action: "Left Click",
            active: true
        },
        {
            id: "point",
            gesture: "point",
            title: "Point",
            action: "Right Click",
            active: true
        },
        {
            id: "thumb_up",
            gesture: "up",
            title: "Thumbs Up",
            action: "Scroll Up",
            active: true
        },
        {
            id: "thumb_down",
            gesture: "down",
            title: "Thumbs Down",
            action: "Scroll Down",
            active: true
        },
        {
            id: "fist",
            gesture: "fist",
            title: "Fist",
            action: "Pause / Resume",
            active: true
        }
    ]

    // Mock M4 mapping rows.
    property var mappingRows: [
        { id: "right_pinch_left_mouse", hand: "Right", source: "PINCH", type: "Gesture", action: "Left click", output: "Mouse", behavior: "Hold", state: "Enabled", enabled: true },
        { id: "right_fist_right_mouse", hand: "Right", source: "FIST", type: "Gesture", action: "Right click", output: "Mouse", behavior: "Press", state: "Disabled", enabled: false },
        { id: "left_pinch_space", hand: "Left", source: "PINCH", type: "Gesture", action: "Key Space", output: "Keyboard", behavior: "Press", state: "Disabled", enabled: false }
    ]

    property var gestureLibrary: [
        {
            id: "open_hand",
            gesture: "open",
            title: "Open Hand",
            category: "Static",
            description: "Open hand with fingers extended",
            enabled: true
        },
        {
            id: "pinch",
            gesture: "pinch",
            title: "Pinch",
            category: "Static",
            description: "Thumb and index finger pinch",
            enabled: true
        },
        {
            id: "point",
            gesture: "point",
            title: "Point",
            category: "Static",
            description: "Index finger extended for pointing",
            enabled: true
        },
        {
            id: "fist",
            gesture: "fist",
            title: "Fist",
            category: "Static",
            description: "Closed hand",
            enabled: true
        },
        {
            id: "thumb_up",
            gesture: "up",
            title: "Thumbs Up",
            category: "Static",
            description: "Thumb pointing upward",
            enabled: true
        },
        {
            id: "thumb_down",
            gesture: "down",
            title: "Thumbs Down",
            category: "Static",
            description: "Thumb pointing downward",
            enabled: true
        },
        {
            id: "swipe_horizontal",
            gesture: "swipe",
            title: "Swipe Left / Right",
            category: "Dynamic",
            description: "Horizontal hand motion",
            enabled: false
        },
        {
            id: "swipe_vertical",
            gesture: "swipe",
            title: "Swipe Up / Down",
            category: "Dynamic",
            description: "Vertical hand motion",
            enabled: false
        },
        {
            id: "two_hand_rotate",
            gesture: "rotate",
            title: "Two-Hand Rotate",
            category: "Two-hand",
            description: "Relative rotation of both hands",
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
        outputArmed = value && pipelineRunning
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

    function setDebounceMs(value) { debounceMs = Math.round(value) }
    function setCooldownMs(value) { cooldownMs = Math.round(value) }
    function setRequireRelease(value) { requireRelease = value }
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

    function setPipelineRunning(enabled) {
        cameraRunning = enabled
        pipelineRunning = enabled
        if (!enabled)
            outputArmed = false
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
