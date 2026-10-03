import QtQuick

QtObject {
    id: root

    // This object is the mock implementation of the UI state contract. Runtime
    // services can replace it later without forcing the QML pages to know where
    // camera, tracking, ML or output data comes from.
    property bool cameraRunning: true
    property bool pipelineRunning: true
    property bool outputArmed: false

    property string activeProfile: "Default"
    property var profileNames: ["Default", "Desktop", "Presentation", "Game"]
    property string outputMode: "Mouse / Keyboard"

    property int sensitivity: 70
    property bool smoothing: true
    property int deadzone: 12

    property int detectionConfidence: 60
    property int trackingConfidence: 55
    property int recognitionThreshold: 80
    property int debounceMs: 120
    property int cooldownMs: 250
    property bool requireRelease: true
    property bool showLandmarks: true
    property bool mirrorPreview: false
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
    property string resolution: "640 × 480"
    property int latencyMs: pipelineRunning ? 12 : 0
    property int cursorX: pipelineRunning ? 1280 : 0
    property int cursorY: pipelineRunning ? 720 : 0
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

    // Display labels stay human-readable, while stable IDs are already present
    // for future profile JSON and C++ enums.
    property var mappingRows: [
        {
            id: "right_move_cursor",
            handId: "right",
            hand: "Right",
            sourceId: "open_hand",
            source: "Open Hand",
            typeId: "continuous",
            type: "Continuous",
            actionId: "mouse.move",
            action: "Move cursor",
            outputId: "mouse",
            output: "Mouse",
            state: "Enabled"
        },
        {
            id: "right_left_click",
            handId: "right",
            hand: "Right",
            sourceId: "pinch",
            source: "Pinch",
            typeId: "gesture",
            type: "Gesture",
            actionId: "mouse.left_click",
            action: "Left click",
            outputId: "mouse",
            output: "Mouse",
            state: "Enabled"
        },
        {
            id: "right_right_click",
            handId: "right",
            hand: "Right",
            sourceId: "point",
            source: "Point",
            typeId: "gesture",
            type: "Gesture",
            actionId: "mouse.right_click",
            action: "Right click",
            outputId: "mouse",
            output: "Mouse",
            state: "Enabled"
        },
        {
            id: "left_scroll_up",
            handId: "left",
            hand: "Left",
            sourceId: "thumb_up",
            source: "Thumbs Up",
            typeId: "gesture",
            type: "Gesture",
            actionId: "mouse.scroll_up",
            action: "Scroll up",
            outputId: "mouse",
            output: "Mouse",
            state: "Enabled"
        },
        {
            id: "left_scroll_down",
            handId: "left",
            hand: "Left",
            sourceId: "thumb_down",
            source: "Thumbs Down",
            typeId: "gesture",
            type: "Gesture",
            actionId: "mouse.scroll_down",
            action: "Scroll down",
            outputId: "mouse",
            output: "Mouse",
            state: "Enabled"
        },
        {
            id: "either_pause_resume",
            handId: "either",
            hand: "Either",
            sourceId: "fist",
            source: "Fist",
            typeId: "gesture",
            type: "Gesture",
            actionId: "controller.pause_resume",
            action: "Pause / resume",
            outputId: "controller",
            output: "Controller",
            state: "Enabled"
        }
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
