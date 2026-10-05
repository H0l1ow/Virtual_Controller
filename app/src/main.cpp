#include "runtime/RuntimeController.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(
        QStringLiteral("VirtualController"));

    QCoreApplication::setApplicationName(
        QStringLiteral("VirtualController"));

    QCoreApplication::setApplicationVersion(
        QStringLiteral("0.5.0"));

    QQuickStyle::setStyle(
        QStringLiteral("Basic"));

    vc::RuntimeController runtimeController;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        QStringLiteral("runtimeController"),
        &runtimeController);

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
    return app.exec();
}
