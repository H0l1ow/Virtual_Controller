import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "theme"
import "mock"
import "components"
import "pages"

ApplicationWindow {
    id: window

    width: 1440
    height: 900
    minimumWidth: 1220
    minimumHeight: 760
    visible: true
    title: "Virtual Controller"
    color: theme.background
    flags: Qt.Window | Qt.FramelessWindowHint

    Theme {
        id: theme
    }
    UiMock {
        id: uiStateMock
    }

    property int currentPage: 0

    Rectangle {
        anchors.fill: parent
        color: theme.background
        border.width: 1
        border.color: theme.borderStrong
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        AppTopBar {
            Layout.fillWidth: true
            Layout.preferredHeight: theme.topBarHeight
            theme: theme
            windowRef: window
            currentIndex: window.currentPage
            onPageSelected: function(index) {
                window.currentPage = index
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.currentPage

            ControllerPage {
                theme: theme
                uiState: uiStateMock
            }
            MappingPage {
                theme: theme
                uiState: uiStateMock
            }
            GesturesPage {
                theme: theme
                uiState: uiStateMock
            }
            GamepadPage {
                theme: theme
                uiState: uiStateMock
            }
            SettingsPage {
                theme: theme
                uiState: uiStateMock
            }
        }
    }

    // Thin native resize zones keep the frameless desktop window practical.
    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: window.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: window.startSystemResize(Qt.BottomEdge)
    }
    // Corner hit areas take precedence over the straight edge zones so the
    // frameless window behaves like a normal desktop window at all corners.
    MouseArea {
        z: 3
        anchors.left: parent.left
        anchors.top: parent.top
        width: 8
        height: 8
        cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }
    MouseArea {
        z: 3
        anchors.right: parent.right
        anchors.top: parent.top
        width: 8
        height: 8
        cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }
    MouseArea {
        z: 3
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 8
        height: 8
        cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }
    MouseArea {
        z: 3
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 8
        height: 8
        cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }

}
