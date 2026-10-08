import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState

    property int selectedMappingPosition: 0
    property int selectedExerciseIndex: 0

    property bool testStarted: false
    property string testStatus: "Ready"
    property string lastResult: ""
    property int progress: 0
    property int targetIndex: 0
    readonly property int clickTargetCount: 5

    property bool dragActive: false
    property bool freezeActive: false
    property real freezeStartX: 0
    property real freezeStartY: 0
    property real freezeLastX: 0
    property real freezeLastY: 0
    property real freezeMaxMove: 0
    property real freezeReleaseJump: 0

    property bool previousLeftPressed: false
    property bool previousRightPressed: false
    property bool previousFrozen: false
    property bool previousKeyPressed: false
    property real previousWheel: 0
    property double lastScrollAcceptedMs: 0

    property bool runAllActive: false
    property var runQueue: []
    property int runQueuePosition: -1
    property int runPassed: 0
    property int runFailed: 0

    readonly property var targetPositions: [
        { x: 0.18, y: 0.24 },
        { x: 0.78, y: 0.22 },
        { x: 0.72, y: 0.72 },
        { x: 0.24, y: 0.76 },
        { x: 0.50, y: 0.47 }
    ]

    function exercisesFor(row) {
        if (!row)
            return []

        if (row.action === "Left click") {
            const result = ["Click targets"]
            if (row.behavior === "Hold")
                result.push("Drag & Drop")
            return result
        }
        if (row.action === "Right click")
            return ["Right-click targets"]
        if (row.action === "Freeze cursor")
            return ["Freeze / reposition"]
        if (row.action === "Scroll up" || row.action === "Scroll down")
            return ["Scroll steps"]
        if (row.action && row.action.indexOf("Key ") === 0)
            return ["Key action"]
        return []
    }

    function buildTestableMappings() {
        const rows = root.uiState.mappingRows || []
        const result = []
        for (let i = 0; i < rows.length; ++i) {
            const row = rows[i]
            if (!row.enabled)
                continue
            const exercises = root.exercisesFor(row)
            if (exercises.length === 0)
                continue
            result.push({
                rowIndex: i,
                row: row,
                label: row.hand + " + " + row.source + "  →  " + row.action + "  /  " + row.behavior
            })
        }
        return result
    }

    readonly property var testableMappings: root.buildTestableMappings()
    readonly property var mappingLabels: {
        const values = []
        for (let i = 0; i < root.testableMappings.length; ++i)
            values.push(root.testableMappings[i].label)
        return values
    }

    function selectedEntry() {
        if (root.testableMappings.length === 0)
            return null
        const index = Math.max(0, Math.min(root.selectedMappingPosition, root.testableMappings.length - 1))
        return root.testableMappings[index]
    }

    function selectedMapping() {
        const entry = root.selectedEntry()
        return entry ? entry.row : null
    }

    function currentExercises() {
        return root.exercisesFor(root.selectedMapping())
    }

    function currentExercise() {
        const values = root.currentExercises()
        if (values.length === 0)
            return ""
        return values[Math.max(0, Math.min(root.selectedExerciseIndex, values.length - 1))]
    }

    function liveGestureFor(row) {
        if (!row)
            return "NONE"
        return row.hand === "Left" ? root.uiState.leftGesture : root.uiState.rightGesture
    }

    function liveEventFor(row) {
        if (!row)
            return "IDLE"
        return row.hand === "Left" ? root.uiState.leftGestureEvent : root.uiState.rightGestureEvent
    }

    function gestureMatches(row) {
        return !!row && root.liveGestureFor(row) === row.source
    }

    function logicalKeyName(row) {
        if (!row || !row.action || row.action.indexOf("Key ") !== 0)
            return ""
        return row.action.substring(4)
    }

    function logicalKeyPressed(row) {
        const keyName = root.logicalKeyName(row)
        if (keyName.length === 0)
            return false
        const values = root.uiState.logicalKeys || []
        return values.indexOf(keyName) !== -1
    }

    function logicalKeysText() {
        const values = root.uiState.logicalKeys || []
        if (values.length === 0)
            return "none"
        const copy = []
        for (let i = 0; i < values.length; ++i)
            copy.push(values[i])
        return copy.join(", ")
    }

    function cursorBoardX() {
        const screenWidth = Math.max(1, root.uiState.cursorScreenWidth - 1)
        return exerciseBoard.width * Math.max(0, Math.min(1, root.uiState.cursorX / screenWidth))
    }

    function cursorBoardY() {
        const screenHeight = Math.max(1, root.uiState.cursorScreenHeight - 1)
        return exerciseBoard.height * Math.max(0, Math.min(1, root.uiState.cursorY / screenHeight))
    }

    function distance(ax, ay, bx, by) {
        const dx = ax - bx
        const dy = ay - by
        return Math.sqrt(dx * dx + dy * dy)
    }

    function cursorInsideNormalized(nx, ny, radius) {
        return root.distance(
            root.cursorBoardX(), root.cursorBoardY(),
            exerciseBoard.width * nx, exerciseBoard.height * ny) <= radius
    }

    function resetExerciseState() {
        root.progress = 0
        root.targetIndex = 0
        root.dragActive = false
        root.freezeActive = false
        root.freezeMaxMove = 0
        root.freezeReleaseJump = 0
        root.previousLeftPressed = root.uiState.logicalMouseLeft
        root.previousRightPressed = root.uiState.logicalMouseRight
        root.previousFrozen = root.uiState.cursorFrozen
        root.previousKeyPressed = root.logicalKeyPressed(root.selectedMapping())
        root.previousWheel = root.uiState.logicalWheel
        root.lastScrollAcceptedMs = 0
        root.lastResult = ""
    }

    function startCurrentTest() {
        const row = root.selectedMapping()
        const exercise = root.currentExercise()
        if (!row || exercise.length === 0) {
            root.testStatus = "No testable mapping selected"
            root.testStarted = false
            return
        }
        if (!root.uiState.pipelineRunning) {
            root.testStatus = "Start tracking first"
            root.testStarted = false
            return
        }

        root.resetExerciseState()
        root.testStarted = true
        root.testStatus = "Running"
    }

    function stopTest() {
        root.testStarted = false
        if (!root.runAllActive)
            root.testStatus = "Ready"
    }

    function completeCurrentTest(message) {
        if (!root.testStarted)
            return
        root.testStarted = false
        root.testStatus = "Passed"
        root.lastResult = message

        if (root.runAllActive) {
            root.runPassed += 1
            advanceTimer.restart()
        }
    }

    function failCurrentRunAll(message) {
        if (!root.runAllActive)
            return
        root.testStarted = false
        root.testStatus = "Failed"
        root.lastResult = message
        root.runFailed += 1
        advanceTimer.restart()
    }

    function buildRunQueue() {
        const queue = []
        for (let mappingPosition = 0; mappingPosition < root.testableMappings.length; ++mappingPosition) {
            const row = root.testableMappings[mappingPosition].row
            const exercises = root.exercisesFor(row)
            for (let exerciseIndex = 0; exerciseIndex < exercises.length; ++exerciseIndex) {
                queue.push({
                    mappingPosition: mappingPosition,
                    exerciseIndex: exerciseIndex,
                    label: root.testableMappings[mappingPosition].label + " — " + exercises[exerciseIndex]
                })
            }
        }
        return queue
    }

    function startRunAll() {
        const queue = root.buildRunQueue()
        if (queue.length === 0) {
            root.testStatus = "No enabled testable mappings"
            return
        }
        if (!root.uiState.pipelineRunning) {
            root.testStatus = "Start tracking first"
            return
        }

        root.runQueue = queue
        root.runQueuePosition = 0
        root.runPassed = 0
        root.runFailed = 0
        root.runAllActive = true
        root.applyRunQueueEntry()
    }

    function applyRunQueueEntry() {
        if (!root.runAllActive)
            return
        if (root.runQueuePosition < 0 || root.runQueuePosition >= root.runQueue.length) {
            root.runAllActive = false
            root.testStarted = false
            root.testStatus = "Run complete"
            root.lastResult = root.runPassed + " passed, " + root.runFailed + " failed"
            return
        }

        const entry = root.runQueue[root.runQueuePosition]
        root.selectedMappingPosition = entry.mappingPosition
        root.selectedExerciseIndex = entry.exerciseIndex
        root.startCurrentTest()
    }

    function cancelRunAll() {
        root.runAllActive = false
        root.testStarted = false
        root.runQueue = []
        root.runQueuePosition = -1
        root.testStatus = "Ready"
    }

    function pollLogicalState() {
        if (!root.testStarted)
            return

        const row = root.selectedMapping()
        if (!row)
            return

        const exercise = root.currentExercise()
        const leftPressed = root.uiState.logicalMouseLeft
        const rightPressed = root.uiState.logicalMouseRight
        const frozen = root.uiState.cursorFrozen
        const keyPressed = root.logicalKeyPressed(row)
        const wheel = root.uiState.logicalWheel
        const matches = root.gestureMatches(row)

        const leftPressEdge = leftPressed && !root.previousLeftPressed
        const leftReleaseEdge = !leftPressed && root.previousLeftPressed
        const rightPressEdge = rightPressed && !root.previousRightPressed
        const frozenEdge = frozen && !root.previousFrozen
        const unfrozenEdge = !frozen && root.previousFrozen
        const keyPressEdge = keyPressed && !root.previousKeyPressed

        if (exercise === "Click targets") {
            if (leftPressEdge && matches) {
                const target = root.targetPositions[root.targetIndex % root.targetPositions.length]
                if (root.cursorInsideNormalized(target.x, target.y, 38)) {
                    root.progress += 1
                    root.targetIndex += 1
                    root.lastResult = "Target " + root.progress + " / " + root.clickTargetCount
                    if (root.progress >= root.clickTargetCount)
                        root.completeCurrentTest("Left-click targets completed")
                } else {
                    root.lastResult = "Left click detected, but cursor was outside the target"
                }
            }
        } else if (exercise === "Right-click targets") {
            if (rightPressEdge && matches) {
                const target = root.targetPositions[root.targetIndex % root.targetPositions.length]
                if (root.cursorInsideNormalized(target.x, target.y, 38)) {
                    root.progress += 1
                    root.targetIndex += 1
                    root.lastResult = "Target " + root.progress + " / " + root.clickTargetCount
                    if (root.progress >= root.clickTargetCount)
                        root.completeCurrentTest("Right-click targets completed")
                } else {
                    root.lastResult = "Right click detected, but cursor was outside the target"
                }
            }
        } else if (exercise === "Drag & Drop") {
            if (!root.dragActive && leftPressEdge && matches) {
                if (root.cursorInsideNormalized(0.22, 0.50, 54)) {
                    root.dragActive = true
                    root.lastResult = "Object grabbed — keep the gesture held"
                } else {
                    root.lastResult = "Press detected outside the source object"
                }
            }

            if (root.dragActive && leftReleaseEdge) {
                if (root.cursorInsideNormalized(0.78, 0.50, 66)) {
                    root.dragActive = false
                    root.completeCurrentTest("Drag & Drop completed")
                } else {
                    root.dragActive = false
                    root.lastResult = "Released outside the DROP zone — try again"
                }
            }
        } else if (exercise === "Freeze / reposition") {
            if (frozenEdge && matches) {
                root.freezeActive = true
                root.freezeStartX = root.uiState.cursorX
                root.freezeStartY = root.uiState.cursorY
                root.freezeLastX = root.uiState.cursorX
                root.freezeLastY = root.uiState.cursorY
                root.freezeMaxMove = 0
                root.freezeReleaseJump = 0
                root.lastResult = "Frozen — reposition the hand, then release the gesture"
            }

            if (root.freezeActive && frozen) {
                const moved = root.distance(
                    root.freezeStartX, root.freezeStartY,
                    root.uiState.cursorX, root.uiState.cursorY)
                root.freezeMaxMove = Math.max(root.freezeMaxMove, moved)
                root.freezeLastX = root.uiState.cursorX
                root.freezeLastY = root.uiState.cursorY
            }

            if (root.freezeActive && unfrozenEdge) {
                root.freezeReleaseJump = root.distance(
                    root.freezeLastX, root.freezeLastY,
                    root.uiState.cursorX, root.uiState.cursorY)
                const allowedMove = 8
                const allowedReleaseJump = Math.max(60, root.uiState.cursorScreenWidth * 0.05)
                root.freezeActive = false
                if (root.freezeMaxMove <= allowedMove && root.freezeReleaseJump <= allowedReleaseJump) {
                    root.completeCurrentTest(
                        "Freeze stable; release jump " + Math.round(root.freezeReleaseJump) + " px")
                } else {
                    root.lastResult = "Freeze moved " + Math.round(root.freezeMaxMove)
                        + " px; release jump " + Math.round(root.freezeReleaseJump) + " px — try again"
                }
            }
        } else if (exercise === "Scroll steps") {
            const expectedDirection = row.action === "Scroll up" ? 1 : -1
            const now = Date.now()
            if (matches && wheel * expectedDirection > 0.25 && now - root.lastScrollAcceptedMs >= 140) {
                root.lastScrollAcceptedMs = now
                root.progress += 1
                root.lastResult = "Scroll step " + root.progress + " / 5"
                if (root.progress >= 5)
                    root.completeCurrentTest(row.action + " completed")
            } else if (matches && wheel * expectedDirection < -0.25) {
                root.lastResult = "Wrong scroll direction detected"
            }
        } else if (exercise === "Key action") {
            if (keyPressEdge && matches)
                root.completeCurrentTest(row.action + " detected")
        }

        root.previousLeftPressed = leftPressed
        root.previousRightPressed = rightPressed
        root.previousFrozen = frozen
        root.previousKeyPressed = keyPressed
        root.previousWheel = wheel
    }

    Timer {
        id: pollTimer
        interval: 16
        repeat: true
        running: root.visible && root.testStarted
        onTriggered: root.pollLogicalState()
    }

    Timer {
        id: advanceTimer
        interval: 650
        repeat: false
        onTriggered: {
            if (!root.runAllActive)
                return
            root.runQueuePosition += 1
            root.applyRunQueueEntry()
        }
    }

    onSelectedMappingPositionChanged: {
        root.selectedExerciseIndex = 0
        if (!root.runAllActive) {
            root.testStarted = false
            root.resetExerciseState()
            root.testStatus = "Ready"
        }
    }

    onSelectedExerciseIndexChanged: {
        if (!root.runAllActive) {
            root.testStarted = false
            root.resetExerciseState()
            root.testStatus = "Ready"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.theme.pageMargin
        spacing: 12

        PageToolbar {
            Layout.fillWidth: true
            theme: root.theme
            title: "Gesture Playground"
            subtitle: "Test enabled Mapping rules inside the app before relying on Windows output."

            Text {
                text: root.uiState.activeProfile
                color: root.theme.textSecondary
                font.family: root.theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 26
                color: root.theme.border
            }

            Text {
                text: root.uiState.outputArmed ? "SYSTEM OUTPUT" : "SAFE INTERNAL"
                color: root.uiState.outputArmed ? root.theme.warning : root.theme.success
                font.family: root.theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 0.7
            }

            VcSwitch {
                theme: root.theme
                checked: root.uiState.outputArmed
                enabled: root.uiState.pipelineRunning && root.uiState.outputAvailable
                onToggled: root.uiState.setOutputArmed(checked)
            }

            VcButton {
                theme: root.theme
                text: root.runAllActive ? "Cancel run" : "Run all"
                iconName: root.runAllActive ? "reset" : "play"
                enabled: root.testableMappings.length > 0 && root.uiState.pipelineRunning
                onClicked: {
                    if (root.runAllActive)
                        root.cancelRunAll()
                    else
                        root.startRunAll()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            VcCard {
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        title: "Exercise setup"
                        statusText: root.testableMappings.length + " mappings"
                        statusKind: root.testableMappings.length > 0 ? "success" : "warning"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "MAPPING"
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                    }

                    VcComboBox {
                        id: mappingSelector
                        Layout.fillWidth: true
                        theme: root.theme
                        model: root.mappingLabels
                        currentIndex: root.testableMappings.length === 0
                            ? -1
                            : Math.min(root.selectedMappingPosition, root.testableMappings.length - 1)
                        enabled: root.testableMappings.length > 0 && !root.runAllActive
                        onActivated: function(index) {
                            root.selectedMappingPosition = index
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "EXERCISE"
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                    }

                    VcComboBox {
                        id: exerciseSelector
                        Layout.fillWidth: true
                        theme: root.theme
                        model: root.currentExercises()
                        currentIndex: model.length === 0
                            ? -1
                            : Math.min(root.selectedExerciseIndex, model.length - 1)
                        enabled: model.length > 0 && !root.runAllActive
                        onActivated: function(index) {
                            root.selectedExerciseIndex = index
                        }
                    }

                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: root.testStarted ? "Restart test" : "Start test"
                        iconName: "play"
                        enabled: root.testableMappings.length > 0
                            && root.uiState.pipelineRunning
                            && !root.runAllActive
                        onClicked: root.startCurrentTest()
                    }

                    VcButton {
                        Layout.fillWidth: true
                        theme: root.theme
                        text: "Mark failed / next"
                        variant: "danger"
                        iconName: "reset"
                        visible: root.runAllActive
                        onClicked: root.failCurrentRunAll("Skipped by user")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.theme.borderSubtle
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        Text {
                            Layout.fillWidth: true
                            text: "How it works"
                            color: root.theme.textSecondary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Safe Internal reads ControllerState after GestureStateManager and ActionMapper, but does not require Windows SendInput. Enable System output above only when you want an end-to-end OS check."
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Text {
                        Layout.fillWidth: true
                        visible: root.testableMappings.length === 0
                        text: "No enabled testable mappings. Enable or add a rule on the Mapping page first."
                        color: root.theme.warning
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                }
            }

            VcCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                theme: root.theme

                Item {
                    id: exerciseBoard
                    anchors.fill: parent
                    anchors.margins: 14
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        color: root.theme.backgroundAlt
                        border.width: 1
                        border.color: root.theme.border
                        radius: root.theme.cardRadius
                    }

                    // Subtle grid helps estimate cursor motion without becoming
                    // visual noise during click/freeze tests.
                    Canvas {
                        anchors.fill: parent
                        opacity: 0.22
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = root.theme.borderStrong
                            ctx.lineWidth = 1
                            for (let x = 40; x < width; x += 40) {
                                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
                            }
                            for (let y = 40; y < height; y += 40) {
                                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
                            }
                        }
                    }

                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 20
                        spacing: 4
                        z: 5

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.currentExercise().length > 0 ? root.currentExercise() : "No exercise"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: {
                                if (!root.testStarted)
                                    return "Choose an enabled mapping and start the test"
                                if (root.currentExercise() === "Click targets")
                                    return "Move the logical cursor onto the target and perform the mapped left-click gesture"
                                if (root.currentExercise() === "Right-click targets")
                                    return "Move onto the target and perform the mapped right-click gesture"
                                if (root.currentExercise() === "Drag & Drop")
                                    return root.dragActive ? "Keep holding and move to DROP" : "Grab the object with the mapped Hold action"
                                if (root.currentExercise() === "Freeze / reposition")
                                    return root.freezeActive ? "Reposition your hand, then release the freeze gesture" : "Activate the mapped Freeze cursor gesture"
                                if (root.currentExercise() === "Scroll steps")
                                    return "Perform five mapped scroll steps in the requested direction"
                                if (root.currentExercise() === "Key action")
                                    return "Perform the mapped gesture once"
                                return ""
                            }
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                    }

                    // Click target exercise.
                    Rectangle {
                        visible: root.testStarted
                            && (root.currentExercise() === "Click targets"
                                || root.currentExercise() === "Right-click targets")
                        width: 62
                        height: 62
                        radius: 31
                        x: exerciseBoard.width * root.targetPositions[root.targetIndex % root.targetPositions.length].x - width / 2
                        y: exerciseBoard.height * root.targetPositions[root.targetIndex % root.targetPositions.length].y - height / 2
                        color: Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.12)
                        border.width: 2
                        border.color: root.theme.accent

                        Rectangle {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            radius: 6
                            color: root.theme.accent
                        }
                    }

                    // Drag source. While held, the object follows the logical
                    // cursor so release semantics can be checked internally.
                    Rectangle {
                        visible: root.testStarted && root.currentExercise() === "Drag & Drop"
                        width: 76
                        height: 76
                        radius: 12
                        x: root.dragActive ? root.cursorBoardX() - width / 2 : exerciseBoard.width * 0.22 - width / 2
                        y: root.dragActive ? root.cursorBoardY() - height / 2 : exerciseBoard.height * 0.50 - height / 2
                        color: root.dragActive
                            ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.22)
                            : root.theme.surfaceRaised
                        border.width: 2
                        border.color: root.dragActive ? root.theme.accent : root.theme.borderStrong
                        z: 3

                        Text {
                            anchors.centerIn: parent
                            text: "DRAG"
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Bold
                        }
                    }

                    Rectangle {
                        visible: root.testStarted && root.currentExercise() === "Drag & Drop"
                        width: 112
                        height: 112
                        radius: 14
                        x: exerciseBoard.width * 0.78 - width / 2
                        y: exerciseBoard.height * 0.50 - height / 2
                        color: "transparent"
                        border.width: 2
                        border.color: root.theme.accentDim

                        Text {
                            anchors.centerIn: parent
                            text: "DROP"
                            color: root.theme.accent
                            font.family: root.theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Bold
                        }
                    }

                    Column {
                        visible: root.testStarted && root.currentExercise() === "Freeze / reposition"
                        anchors.centerIn: parent
                        spacing: 12

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 132
                            height: 132
                            radius: 66
                            color: root.uiState.cursorFrozen
                                ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.16)
                                : root.theme.surfaceRaised
                            border.width: 2
                            border.color: root.uiState.cursorFrozen ? root.theme.accent : root.theme.borderStrong

                            Text {
                                anchors.centerIn: parent
                                text: root.uiState.cursorFrozen ? "FROZEN" : "FREEZE"
                                color: root.uiState.cursorFrozen ? root.theme.accent : root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Max movement: " + Math.round(root.freezeMaxMove) + " px   •   Release jump: " + Math.round(root.freezeReleaseJump) + " px"
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pixelSize: 10
                        }
                    }

                    Column {
                        visible: root.testStarted && root.currentExercise() === "Scroll steps"
                        anchors.centerIn: parent
                        spacing: 14

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: {
                                const row = root.selectedMapping()
                                return row ? row.action.toUpperCase() : "SCROLL"
                            }
                            color: root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 22
                            font.weight: Font.Bold
                        }
                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 10
                            Repeater {
                                model: 5
                                delegate: Rectangle {
                                    required property int index
                                    width: 34
                                    height: 8
                                    radius: 4
                                    color: index < root.progress ? root.theme.accent : root.theme.borderStrong
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: root.testStarted && root.currentExercise() === "Key action"
                        anchors.centerIn: parent
                        width: 210
                        height: 110
                        radius: 12
                        color: root.theme.surfaceRaised
                        border.width: 2
                        border.color: root.logicalKeyPressed(root.selectedMapping()) ? root.theme.accent : root.theme.borderStrong

                        Text {
                            anchors.centerIn: parent
                            text: root.logicalKeyName(root.selectedMapping())
                            color: root.logicalKeyPressed(root.selectedMapping()) ? root.theme.accent : root.theme.textPrimary
                            font.family: root.theme.fontFamily
                            font.pixelSize: 24
                            font.weight: Font.Bold
                        }
                    }

                    // Logical cursor used by Safe Internal mode. It is driven
                    // by the same ControllerState as Windows output but never
                    // needs to move the physical OS pointer.
                    Item {
                        visible: root.uiState.pipelineRunning
                        width: 26
                        height: 26
                        x: root.cursorBoardX() - width / 2
                        y: root.cursorBoardY() - height / 2
                        z: 10

                        Rectangle {
                            x: 12
                            y: 0
                            width: 2
                            height: 26
                            color: root.uiState.cursorFrozen ? root.theme.warning : root.theme.textPrimary
                        }
                        Rectangle {
                            x: 0
                            y: 12
                            width: 26
                            height: 2
                            color: root.uiState.cursorFrozen ? root.theme.warning : root.theme.textPrimary
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 6
                            height: 6
                            radius: 3
                            color: root.theme.accent
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 14
                        height: 42
                        radius: root.theme.controlRadius
                        color: root.testStatus === "Passed"
                            ? Qt.rgba(root.theme.success.r, root.theme.success.g, root.theme.success.b, 0.10)
                            : root.testStatus === "Failed"
                                ? Qt.rgba(root.theme.error.r, root.theme.error.g, root.theme.error.b, 0.10)
                                : root.theme.surfaceRaised
                        border.width: 1
                        border.color: root.testStatus === "Passed"
                            ? root.theme.success
                            : root.testStatus === "Failed"
                                ? root.theme.error
                                : root.theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            StatusBadge {
                                theme: root.theme
                                text: root.testStatus.toUpperCase()
                                kind: root.testStatus === "Passed" ? "success"
                                    : root.testStatus === "Failed" ? "error"
                                    : root.testStatus === "Running" ? "warning"
                                    : "neutral"
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.lastResult.length > 0 ? root.lastResult : "Waiting for test input"
                                color: root.theme.textSecondary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.currentExercise() === "Click targets"
                                    || root.currentExercise() === "Right-click targets"
                                    || root.currentExercise() === "Scroll steps"
                                text: root.progress + " / " + (root.currentExercise() === "Scroll steps" ? 5 : root.clickTargetCount)
                                color: root.theme.textPrimary
                                font.family: root.theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }
                    }
                }
            }

            VcCard {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                theme: root.theme

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    PanelHeader {
                        Layout.fillWidth: true
                        theme: root.theme
                        title: "Live diagnostics"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Classifier → event → mapped logical action"
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8

                        Text { text: "Hand"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.selectedMapping() ? root.selectedMapping().hand : "-"; color: root.theme.textPrimary; font.family: root.theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                        Text { text: "Expected"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.selectedMapping() ? root.selectedMapping().source : "-"; color: root.theme.textPrimary; font.family: root.theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                        Text { text: "Recognized"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.liveGestureFor(root.selectedMapping()); color: root.gestureMatches(root.selectedMapping()) ? root.theme.success : root.theme.textSecondary; font.family: root.theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                        Text { text: "Event"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.liveEventFor(root.selectedMapping()); color: root.theme.textSecondary; font.family: root.theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                        Text { text: "Mapped action"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.selectedMapping() ? root.selectedMapping().action : "-"; color: root.theme.textPrimary; font.family: root.theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; elide: Text.ElideRight }
                        Text { text: "Behavior"; color: root.theme.textMuted; font.family: root.theme.fontFamily; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: root.selectedMapping() ? root.selectedMapping().behavior : "-"; color: root.theme.textSecondary; font.family: root.theme.fontFamily; font.pixelSize: 11 }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }

                    Text {
                        text: "LOGICAL OUTPUT"
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        StatusBadge { theme: root.theme; text: "LMB"; kind: root.uiState.logicalMouseLeft ? "success" : "neutral" }
                        StatusBadge { theme: root.theme; text: "RMB"; kind: root.uiState.logicalMouseRight ? "success" : "neutral" }
                        StatusBadge { theme: root.theme; text: "FREEZE"; kind: root.uiState.cursorFrozen ? "warning" : "neutral" }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Wheel: " + Number(root.uiState.logicalWheel).toFixed(1)
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Keys: " + root.logicalKeysText()
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Cursor: " + root.uiState.cursorX + " / " + root.uiState.cursorY
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: root.theme.borderSubtle }

                    Text {
                        text: "RUN ALL"
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.runAllActive
                            ? "Exercise " + (root.runQueuePosition + 1) + " / " + root.runQueue.length
                            : "Not running"
                        color: root.theme.textSecondary
                        font.family: root.theme.fontFamily
                        font.pixelSize: 10
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        StatusBadge { theme: root.theme; text: root.runPassed + " passed"; kind: "success" }
                        StatusBadge { theme: root.theme; text: root.runFailed + " failed"; kind: root.runFailed > 0 ? "error" : "neutral" }
                        Item { Layout.fillWidth: true }
                    }

                    Item { Layout.fillHeight: true }

                    Text {
                        Layout.fillWidth: true
                        text: root.uiState.outputArmed
                            ? "System output is armed. Playground actions are also being sent to Windows. F8 remains the emergency disarm."
                            : "Safe Internal mode: the playground validates logical mappings without sending clicks/keys to Windows."
                        color: root.uiState.outputArmed ? root.theme.warning : root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
