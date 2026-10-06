#include "runtime/RuntimeController.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <QtGlobal>

#include <chrono>
#include <cstdlib>
#include <thread>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

namespace {

[[noreturn]] void forceTerminateProcess(
    int exitCode)
{
#ifdef Q_OS_WIN
    ::TerminateProcess(
        ::GetCurrentProcess(),
        static_cast<UINT>(exitCode));

    std::abort();
#else
    std::_Exit(exitCode);
#endif
}


void armExitWatchdog(
    std::chrono::milliseconds timeout)
{
    // This watchdog is armed at the instant the user presses X, before Qt
    // Multimedia, MediaPipe, destructors or aboutToQuit handlers can block the
    // GUI thread. If normal shutdown succeeds the process disappears first and
    // this detached thread disappears with it. If teardown deadlocks, the
    // process is terminated after the bounded grace period.
    std::thread(
        [timeout] {
            std::this_thread::sleep_for(
                timeout);

            // Avoid Qt logging here: if shutdown is deadlocked while holding
            // an internal Qt lock, even a log call could block this watchdog.
            forceTerminateProcess(0);
        })
        .detach();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(
        QStringLiteral("VirtualController"));

    QCoreApplication::setApplicationName(
        QStringLiteral("VirtualController"));

    QCoreApplication::setApplicationVersion(
        QStringLiteral("0.5.1"));

    // A multimedia backend is allowed to own QEventLoopLocker objects. For a
    // desktop controller there is no useful background mode after the only
    // window closes, so a quit lock must never keep the process alive.
    QCoreApplication::setQuitLockEnabled(false);
    app.setQuitOnLastWindowClosed(true);

    QQuickStyle::setStyle(
        QStringLiteral("Basic"));

    vc::RuntimeController runtimeController;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeController"),
        &runtimeController);

    QObject::connect(
        &runtimeController,
        &vc::RuntimeController::applicationExitRequested,
        &app,
        [] {
            qInfo() << "[shutdown] close request reached C++";

            // Arm before any cleanup. This also covers a hang inside
            // QCamera::stop(), QMediaCaptureSession teardown or a native ML
            // destructor.
            armExitWatchdog(
                std::chrono::seconds(3));

            // exit() bypasses an interruptible Quit event. The signal is
            // emitted from QML on the GUI thread, which is the required thread
            // for QCoreApplication::exit().
            QCoreApplication::exit(0);
        },
        Qt::DirectConnection);

    // Keep these for other QML/application exit paths. They no longer perform
    // cleanup; cleanup happens only after app.exec() has returned.
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::exit,
        &app,
        [](int returnCode) {
            QCoreApplication::exit(returnCode);
        },
        Qt::QueuedConnection);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::quit,
        &app,
        [] {
            QCoreApplication::exit(0);
        },
        Qt::QueuedConnection);

    const QUrl url(
        QStringLiteral("qrc:/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    const int exitCode =
        app.exec();

    qInfo() << "[shutdown] app.exec returned" << exitCode;

    // Important: there is intentionally NO RuntimeController::stop() connected
    // to aboutToQuit. aboutToQuit is emitted before app.exec() returns, so one
    // blocking multimedia call there would prevent us from ever reaching the
    // bounded fallback below.
    if (!runtimeController.shutdownForExit(
            std::chrono::seconds(1))) {

        qCritical()
            << "[shutdown] worker did not exit within timeout";

        forceTerminateProcess(
            exitCode);
    }

    qInfo() << "[shutdown] RuntimeController shutdown complete";

    return exitCode;
}
