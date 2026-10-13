#pragma once

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QSystemTrayIcon>

class QAction;
class QMenu;
class QQuickWindow;

namespace vc {
class AppPreferences;
class LanguageManager;
class RuntimeController;

// Windows global shortcuts + OS tray. This service never arms system output.
// Shortcuts are optional: unavailable/reserved registrations are reported, not fatal.
class DesktopIntegration final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)
    Q_PROPERTY(bool globalShortcutsAvailable READ globalShortcutsAvailable CONSTANT)
    Q_PROPERTY(QString shortcutError READ shortcutError NOTIFY shortcutStatusChanged)

public:
    DesktopIntegration(RuntimeController *runtime, AppPreferences *preferences,
                       LanguageManager *language, QObject *parent = nullptr);
    ~DesktopIntegration() override;
    void setMainWindow(QQuickWindow *window);

    bool trayAvailable() const { return trayAvailable_; }
    bool globalShortcutsAvailable() const;
    QString shortcutError() const { return shortcutError_; }
    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           qintptr *result) override;

signals:
    void shortcutStatusChanged();

private:
    void restoreMainWindow();
    void rebindHotkeys();
    void unregisterHotkeys();
    void updateTray();
    void rebuildProfileMenu();
    void nextProfile();

    RuntimeController *runtime_;
    AppPreferences *preferences_;
    LanguageManager *language_;
    QPointer<QQuickWindow> mainWindow_;
    QSystemTrayIcon tray_;
    QMenu *menu_{};
    QMenu *profilesMenu_{};
    QAction *restoreAction_{};
    QAction *startAction_{};
    QAction *stopAction_{};
    QAction *disarmAction_{};
    QAction *quitAction_{};
    bool trayAvailable_{};
    bool mockMode_{};
    QString shortcutError_;
};

} // namespace vc
