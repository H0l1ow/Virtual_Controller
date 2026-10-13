#include "runtime/RuntimeController.hpp"
#include "i18n/LanguageManager.hpp"
#include "runtime/AppPreferences.hpp"
#include "runtime/DesktopIntegration.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QApplication>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
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
    QApplication app(argc, argv);

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
    // Owned by QGuiApplication so the singleton outlives the QML engine.
    auto *languageManager = new vc::LanguageManager(&engine, &app);
    auto *appPreferences = new vc::AppPreferences(&app);
    // Own the integration through QQmlEngine so its singleton remains valid
    // until the QML windows and bindings have been destroyed.
    auto *desktopIntegration = new vc::DesktopIntegration(
        &runtimeController, appPreferences, languageManager, &engine);

    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeController"),
        &runtimeController);

    // Expose localization as a QML singleton, independent of context properties.
    // Register before engine.load(), and keep the C++ object alive through app.exec().
    qmlRegisterSingletonInstance(
        "VirtualController.I18n", 1, 0, "LanguageManager", languageManager);
    qmlRegisterSingletonInstance(
        "VirtualController.Preferences", 1, 0, "AppPreferences", appPreferences);
    qmlRegisterSingletonInstance(
        "VirtualController.Desktop", 1, 0, "DesktopIntegration", desktopIntegration);

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
    if (!engine.rootObjects().isEmpty()) {
        desktopIntegration->setMainWindow(
            qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()));
    }

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
