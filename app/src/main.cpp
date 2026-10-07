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

#ifndef VC_APP_VERSION
#define VC_APP_VERSION "0.0.0"
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
    // Last-resort safety net only. Normal window close now keeps the Qt event
    // loop alive until RuntimeController reaches Stopped/Faulted and emits
    // applicationExitReady().
    std::thread(
        [timeout] {
            std::this_thread::sleep_for(timeout);
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
        QStringLiteral(VC_APP_VERSION));

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
            qInfo()
                << "[shutdown] graceful close requested";

            // If a native runtime never returns, the process still cannot hang
            // forever. This is no longer the normal shutdown mechanism.
            armExitWatchdog(
                std::chrono::seconds(5));
        },
        Qt::DirectConnection);

    QObject::connect(
        &runtimeController,
        &vc::RuntimeController::applicationExitReady,
        &app,
        [] {
            qInfo()
                << "[shutdown] runtime reached safe terminal state";

            QCoreApplication::exit(0);
        },
        Qt::QueuedConnection);

    // Non-window QML exits remain supported. Final bounded cleanup still runs
    // after app.exec() returns.
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
        [] {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    const int exitCode = app.exec();

    qInfo()
        << "[shutdown] app.exec returned"
        << exitCode;

    if (!runtimeController.shutdownForExit(
            std::chrono::seconds(2))) {

        qCritical()
            << "[shutdown] worker did not exit within timeout";

        forceTerminateProcess(exitCode);
    }

    qInfo()
        << "[shutdown] RuntimeController shutdown complete";

    return exitCode;
}
