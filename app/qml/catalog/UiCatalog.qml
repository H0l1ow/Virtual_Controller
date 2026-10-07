import QtQuick

// Static UI catalog only. It deliberately contains no live runtime state so
// the real UI no longer depends on UiMock just to obtain labels/demo rows.
QtObject {
    readonly property var profileNames: [
        "Default", "Desktop", "Presentation", "Game"
    ]

    readonly property var gestureHints: [
        { id: "open_hand", gesture: "open", title: "Open Hand", action: "Move Cursor", active: true },
        { id: "pinch", gesture: "pinch", title: "Pinch", action: "Left Click", active: true },
        { id: "point", gesture: "point", title: "Point", action: "Right Click", active: true },
        { id: "thumb_up", gesture: "up", title: "Thumbs Up", action: "Scroll Up", active: true },
        { id: "thumb_down", gesture: "down", title: "Thumbs Down", action: "Scroll Down", active: true },
        { id: "fist", gesture: "fist", title: "Fist", action: "Pause / Resume", active: true }
    ]

    // Demo/catalog rows only. Real mapping storage is a later milestone.
    readonly property var mappingRows: [
        {
            id: "right_move_cursor", handId: "right", hand: "Right",
            sourceId: "open_hand", source: "Open Hand", typeId: "continuous",
            type: "Continuous", actionId: "mouse.move", action: "Move cursor",
            outputId: "mouse", output: "Mouse", state: "Planned"
        },
        {
            id: "right_left_click", handId: "right", hand: "Right",
            sourceId: "pinch", source: "Pinch", typeId: "gesture",
            type: "Gesture", actionId: "mouse.left_click", action: "Left click",
            outputId: "mouse", output: "Mouse", state: "Planned"
        },
        {
            id: "right_right_click", handId: "right", hand: "Right",
            sourceId: "point", source: "Point", typeId: "gesture",
            type: "Gesture", actionId: "mouse.right_click", action: "Right click",
            outputId: "mouse", output: "Mouse", state: "Planned"
        },
        {
            id: "left_scroll_up", handId: "left", hand: "Left",
            sourceId: "thumb_up", source: "Thumbs Up", typeId: "gesture",
            type: "Gesture", actionId: "mouse.scroll_up", action: "Scroll up",
            outputId: "mouse", output: "Mouse", state: "Planned"
        },
        {
            id: "left_scroll_down", handId: "left", hand: "Left",
            sourceId: "thumb_down", source: "Thumbs Down", typeId: "gesture",
            type: "Gesture", actionId: "mouse.scroll_down", action: "Scroll down",
            outputId: "mouse", output: "Mouse", state: "Planned"
        },
        {
            id: "either_pause_resume", handId: "either", hand: "Either",
            sourceId: "fist", source: "Fist", typeId: "gesture",
            type: "Gesture", actionId: "controller.pause_resume", action: "Pause / resume",
            outputId: "controller", output: "Controller", state: "Planned"
        }
    ]

    readonly property var gestureLibrary: [
        { id: "open_hand", gesture: "open", title: "Open Hand", category: "Static", description: "Open hand with fingers extended", enabled: false },
        { id: "pinch", gesture: "pinch", title: "Pinch", category: "Static", description: "Thumb and index finger pinch", enabled: false },
        { id: "point", gesture: "point", title: "Point", category: "Static", description: "Index finger extended for pointing", enabled: false },
        { id: "fist", gesture: "fist", title: "Fist", category: "Static", description: "Closed hand", enabled: false },
        { id: "thumb_up", gesture: "up", title: "Thumbs Up", category: "Static", description: "Thumb pointing upward", enabled: false },
        { id: "thumb_down", gesture: "down", title: "Thumbs Down", category: "Static", description: "Thumb pointing downward", enabled: false },
        { id: "swipe_horizontal", gesture: "swipe", title: "Swipe Left / Right", category: "Dynamic", description: "Horizontal hand motion", enabled: false },
        { id: "swipe_vertical", gesture: "swipe", title: "Swipe Up / Down", category: "Dynamic", description: "Vertical hand motion", enabled: false },
        { id: "two_hand_rotate", gesture: "rotate", title: "Two-Hand Rotate", category: "Two-hand", description: "Relative rotation of both hands", enabled: false }
    ]
}
