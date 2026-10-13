#include "runtime/DesktopIntegration.hpp"

#include "i18n/LanguageManager.hpp"
#include "runtime/AppPreferences.hpp"
#include "runtime/RuntimeController.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QColor>
#include <QMetaObject>
#include <QPen>
#include <QStringList>
#include <QAbstractEventDispatcher>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QQuickWindow>
#include <QWindow>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

namespace vc {
namespace {
// Thread-based Windows hotkeys are delivered as WM_HOTKEY messages while the
// application's main window is minimized. These IDs do not overlap with F8.
constexpr int kToggleId = 0x5140;
constexpr int kStopId = 0x5141;
constexpr int kProfileId = 0x5142;

QIcon trayIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(32, 42, 56));
    painter.setPen(QPen(QColor(112, 207, 168), 3));
    painter.drawRoundedRect(3, 3, 58, 58, 13, 13);
    painter.setPen(QPen(Qt::white, 5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(17, 24, 27, 42);
    painter.drawLine(27, 42, 47, 18);
    return QIcon(pixmap);
}

#ifdef Q_OS_WIN
int shortcutKey(const QString &text)
{
    if (!text.startsWith(QStringLiteral("Ctrl+Alt+F"))) return 0;
    bool ok = false;
    const int key = text.mid(10).toInt(&ok);
    return ok && key >= 5 && key <= 12 && key != 8
        ? VK_F1 + key - 1 : 0;
}
#endif
} // namespace

DesktopIntegration::DesktopIntegration(RuntimeController *runtime,
                                     AppPreferences *preferences,
                                     LanguageManager *language,
                                     QObject *parent)
    : QObject(parent)
    , runtime_(runtime)
    , preferences_(preferences)
    , language_(language)
    , trayAvailable_(QSystemTrayIcon::isSystemTrayAvailable())
    , mockMode_(QCoreApplication::arguments().contains(QStringLiteral("--mock-ui")))
{
    // Mock UI is a visual sandbox: never bind real global hotkeys or tray
    // commands to the otherwise-live RuntimeController.
    if (mockMode_)
        trayAvailable_ = false;
    menu_ = new QMenu;
    restoreAction_ = menu_->addAction(QString{});
    startAction_ = menu_->addAction(QString{});
    stopAction_ = menu_->addAction(QString{});
    disarmAction_ = menu_->addAction(QString{});
    profilesMenu_ = menu_->addMenu(QString{});
    menu_->addSeparator();
    quitAction_ = menu_->addAction(QString{});

    connect(restoreAction_, &QAction::triggered, this, &DesktopIntegration::restoreMainWindow);
    connect(startAction_, &QAction::triggered, runtime_, &RuntimeController::start);
    connect(stopAction_, &QAction::triggered, this, [this] {
        runtime_->setOutputArmed(false);
        runtime_->stop();
    });
    connect(disarmAction_, &QAction::triggered, this, [this] {
        runtime_->setOutputArmed(false);
    });
    connect(quitAction_, &QAction::triggered, runtime_, &RuntimeController::requestApplicationExit);
    connect(&tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            restoreMainWindow();
    });
    connect(runtime_, &RuntimeController::stateChanged, this, &DesktopIntegration::updateTray);
    connect(runtime_, &RuntimeController::mappingChanged, this, [this] {
        rebuildProfileMenu();
        updateTray();
    });
    connect(language_, &LanguageManager::languageChanged, this, [this] {
        updateTray();
        rebindHotkeys();
    });
    connect(preferences_, &AppPreferences::shortcutsChanged, this, &DesktopIntegration::rebindHotkeys);

    tray_.setIcon(trayIcon());
    tray_.setContextMenu(menu_);
    if (trayAvailable_) tray_.show();

    if (globalShortcutsAvailable()) {
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
        rebindHotkeys();
    }
    rebuildProfileMenu();
    updateTray();
}

DesktopIntegration::~DesktopIntegration()
{
#ifdef Q_OS_WIN
    if (auto *dispatcher = QAbstractEventDispatcher::instance())
        dispatcher->removeNativeEventFilter(this);
#endif
    unregisterHotkeys();
    tray_.hide();
    tray_.setContextMenu(nullptr);
    delete menu_;
}

bool DesktopIntegration::globalShortcutsAvailable() const
{
#ifdef Q_OS_WIN
    return !mockMode_;
#else
    return false;
#endif
}

void DesktopIntegration::setMainWindow(QQuickWindow *window)
{
    mainWindow_ = window;
}

void DesktopIntegration::restoreMainWindow()
{
    if (!mainWindow_) return;
    // Use the QML restore policy: do not force immersive fullscreen from PiP/tray.
    if (!QMetaObject::invokeMethod(mainWindow_, "restoreMainWindow")) {
        mainWindow_->showNormal();
        mainWindow_->raise();
        mainWindow_->requestActivate();
    }
}

void DesktopIntegration::nextProfile()
{
    const QStringList names = runtime_->mappingProfileNames();
    if (names.size() < 2) return;
    const int index = names.indexOf(runtime_->activeProfile());
    runtime_->setActiveProfile(names.at((index + 1) % names.size()));
}

void DesktopIntegration::rebuildProfileMenu()
{
    profilesMenu_->clear();
    for (const QString &name : runtime_->mappingProfileNames()) {
        auto *item = profilesMenu_->addAction(name);
        item->setCheckable(true);
        item->setChecked(name == runtime_->activeProfile());
        connect(item, &QAction::triggered, this, [this, name] {
            runtime_->setActiveProfile(name);
        });
    }
}

void DesktopIntegration::updateTray()
{
    restoreAction_->setText(tr("Restore window"));
    startAction_->setText(tr("Start tracking"));
    stopAction_->setText(tr("Stop tracking"));
    disarmAction_->setText(tr("Disarm input"));
    profilesMenu_->setTitle(tr("Profiles"));
    quitAction_->setText(tr("Exit"));
    startAction_->setEnabled(runtime_->canStart());
    stopAction_->setEnabled(runtime_->canStop());
    disarmAction_->setEnabled(runtime_->outputArmed());
    tray_.setToolTip(tr("Virtual Controller — %1 — %2")
        .arg(runtime_->runtimeStateName(), runtime_->activeProfile()));
}

void DesktopIntegration::unregisterHotkeys()
{
#ifdef Q_OS_WIN
    UnregisterHotKey(nullptr, kToggleId);
    UnregisterHotKey(nullptr, kStopId);
    UnregisterHotKey(nullptr, kProfileId);
#endif
}

void DesktopIntegration::rebindHotkeys()
{
    unregisterHotkeys();
    QStringList errors;
#ifdef Q_OS_WIN
    const QStringList configured{preferences_->shortcutTracking(),
                                 preferences_->shortcutStop(),
                                 preferences_->shortcutNextProfile()};
    const int ids[]{kToggleId, kStopId, kProfileId};
    for (int index = 0; index < configured.size(); ++index) {
        const QString &shortcut = configured.at(index);
        if (shortcut == QStringLiteral("Disabled")) continue;
        if (configured.indexOf(shortcut) != index) {
            errors.append(tr("Duplicate shortcut: %1").arg(shortcut));
            continue;
        }
        const int vk = shortcutKey(shortcut);
        if (!vk || !RegisterHotKey(nullptr, ids[index],
                                   MOD_CONTROL | MOD_ALT | MOD_NOREPEAT,
                                   static_cast<UINT>(vk))) {
            errors.append(tr("Global shortcut unavailable: %1").arg(shortcut));
        }
    }
#endif
    const QString error = errors.join(QStringLiteral("\n"));
    if (error != shortcutError_) {
        shortcutError_ = error;
        emit shortcutStatusChanged();
    }
}

bool DesktopIntegration::nativeEventFilter(const QByteArray &eventType,
                                           void *message, qintptr *result)
{
    Q_UNUSED(result);
#ifdef Q_OS_WIN
    if (eventType == QByteArrayLiteral("windows_dispatcher_MSG")
        || eventType == QByteArrayLiteral("windows_generic_MSG")) {
        auto *msg = static_cast<MSG *>(message);
        if (msg && msg->message == WM_HOTKEY) {
            switch (static_cast<int>(msg->wParam)) {
            case kToggleId:
                runtime_->togglePipeline();
                return true;
            case kStopId:
                runtime_->setOutputArmed(false);
                runtime_->stop();
                return true;
            case kProfileId:
                nextProfile();
                return true;
            default:
                break;
            }
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    return false;
}

} // namespace vc
