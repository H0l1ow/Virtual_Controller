import QtQuick

// Static UI catalog only. It deliberately contains no live runtime state so
// the real UI no longer depends on UiMock just to obtain labels/demo rows.
QtObject {
    readonly property var profileNames: [
        "Default", "Desktop", "Presentation", "Game"
    ]

    readonly property var gestureHints: [
        { id: "right_hand_motion", gesture: "open", title: "Right Hand", action: "Move Cursor", active: true },
        { id: "pinch", gesture: "pinch", title: "Pinch", action: "Mapped action · see Mapping", active: true },
        { id: "point", gesture: "point", title: "Point", action: "Mapped action · see Mapping", active: true },
        { id: "open_hand", gesture: "open", title: "Open Hand", action: "Mapped action · see Mapping", active: true },
        { id: "thumb_up", gesture: "up", title: "Thumbs Up", action: "Planned", active: false },
        { id: "fist", gesture: "fist", title: "Fist", action: "Mapped action · see Mapping", active: true }
    ]

    readonly property var gestureLibrary: [
        { id: "open_hand", gesture: "open", title: "Open Hand", category: "Static", description: "Open hand with fingers extended", enabled: true },
        { id: "pinch", gesture: "pinch", title: "Pinch", category: "Static", description: "Thumb and index finger pinch", enabled: true },
        { id: "point", gesture: "point", title: "Point", category: "Static", description: "Index finger extended for pointing", enabled: true },
        { id: "fist", gesture: "fist", title: "Fist", category: "Static", description: "Closed hand", enabled: true },
        { id: "thumb_up", gesture: "up", title: "Thumbs Up", category: "Static", description: "Thumb pointing upward", enabled: false },
        { id: "thumb_down", gesture: "down", title: "Thumbs Down", category: "Static", description: "Thumb pointing downward", enabled: false },
        { id: "swipe_horizontal", gesture: "swipe", title: "Swipe Left / Right", category: "Dynamic", description: "Horizontal hand motion", enabled: false },
        { id: "swipe_vertical", gesture: "swipe", title: "Swipe Up / Down", category: "Dynamic", description: "Vertical hand motion", enabled: false },
        { id: "two_hand_rotate", gesture: "rotate", title: "Two-Hand Rotate", category: "Two-hand", description: "Relative rotation of both hands", enabled: false }
    ]
}
