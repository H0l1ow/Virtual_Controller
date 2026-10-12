import QtQuick

// Static UI catalog only. It deliberately contains no live runtime state so
// the real UI no longer depends on UiMock just to obtain labels/demo rows.
QtObject {
    readonly property var profileNames: [
        "Default", "Desktop", "Presentation", "Game"
    ]

    readonly property var gestureHints: [
        { id: "right_hand_motion", gesture: "open", title: qsTr("Cursor Movement"), action: qsTr("Requires mapped enable gesture"), active: true },
        { id: "pinch", gesture: "pinch", title: qsTr("Pinch"), action: qsTr("Mapped action · see Mapping"), active: true },
        { id: "point", gesture: "point", title: qsTr("Point"), action: qsTr("Mapped action · see Mapping"), active: true },
        { id: "open_hand", gesture: "open", title: qsTr("Open Hand"), action: qsTr("Mapped action · see Mapping"), active: true },
        { id: "thumb_up", gesture: "up", title: qsTr("Thumbs Up"), action: qsTr("Planned"), active: false },
        { id: "fist", gesture: "fist", title: qsTr("Fist"), action: qsTr("Mapped action · see Mapping"), active: true }
    ]

    readonly property var gestureLibrary: [
        { id: "open_hand", gesture: "open", title: qsTr("Open Hand"), category: "Static", description: qsTr("Open hand with fingers extended"), enabled: true },
        { id: "pinch", gesture: "pinch", title: qsTr("Pinch"), category: "Static", description: qsTr("Thumb and index finger pinch"), enabled: true },
        { id: "point", gesture: "point", title: qsTr("Point"), category: "Static", description: qsTr("Index finger extended for pointing"), enabled: true },
        { id: "fist", gesture: "fist", title: qsTr("Fist"), category: "Static", description: qsTr("Closed hand"), enabled: true },
        { id: "thumb_up", gesture: "up", title: qsTr("Thumbs Up"), category: "Static", description: qsTr("Thumb pointing upward"), enabled: false },
        { id: "thumb_down", gesture: "down", title: qsTr("Thumbs Down"), category: "Static", description: qsTr("Thumb pointing downward"), enabled: false },
        { id: "swipe_horizontal", gesture: "swipe", title: qsTr("Swipe Left / Right"), category: "Dynamic", description: qsTr("Horizontal hand motion"), enabled: false },
        { id: "swipe_vertical", gesture: "swipe", title: qsTr("Swipe Up / Down"), category: "Dynamic", description: qsTr("Vertical hand motion"), enabled: false },
        { id: "two_hand_rotate", gesture: "rotate", title: qsTr("Two-Hand Rotate"), category: "Two-hand", description: qsTr("Relative rotation of both hands"), enabled: false }
    ]
}
