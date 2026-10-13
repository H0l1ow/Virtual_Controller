#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace vc {

// UI appearance is intentionally separate from camera/input runtime state.
// Nothing in this class may arm system input or start a camera.
class AppPreferences final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(QString effectiveTheme READ effectiveTheme NOTIFY effectiveThemeChanged)
    Q_PROPERTY(bool pipEnabled READ pipEnabled WRITE setPipEnabled NOTIFY pipEnabledChanged)
    Q_PROPERTY(QString pipSizeMode READ pipSizeMode WRITE setPipSizeMode NOTIFY pipLayoutChanged)
    Q_PROPERTY(bool pipShowLandmarks READ pipShowLandmarks WRITE setPipShowLandmarks NOTIFY pipLayoutChanged)
    Q_PROPERTY(int pipX READ pipX NOTIFY pipLayoutChanged)
    Q_PROPERTY(int pipY READ pipY NOTIFY pipLayoutChanged)
    Q_PROPERTY(QString shortcutTracking READ shortcutTracking WRITE setShortcutTracking NOTIFY shortcutsChanged)
    Q_PROPERTY(QString shortcutStop READ shortcutStop WRITE setShortcutStop NOTIFY shortcutsChanged)
    Q_PROPERTY(QString shortcutNextProfile READ shortcutNextProfile WRITE setShortcutNextProfile NOTIFY shortcutsChanged)
    Q_PROPERTY(QString saveError READ saveError NOTIFY saveErrorChanged)

public:
    explicit AppPreferences(QObject *parent = nullptr);

    QString themeMode() const { return themeMode_; }
    QString effectiveTheme() const;
    bool pipEnabled() const { return pipEnabled_; }
    QString pipSizeMode() const { return pipSizeMode_; }
    bool pipShowLandmarks() const { return pipShowLandmarks_; }
    int pipX() const { return pipX_; }
    int pipY() const { return pipY_; }
    QString saveError() const { return saveError_; }
    QString shortcutTracking() const { return shortcutTracking_; }
    QString shortcutStop() const { return shortcutStop_; }
    QString shortcutNextProfile() const { return shortcutNextProfile_; }
    static bool validShortcut(const QString &shortcut);
    void setShortcutTracking(const QString &value);
    void setShortcutStop(const QString &value);
    void setShortcutNextProfile(const QString &value);

    void setThemeMode(const QString &mode);
    void setPipEnabled(bool enabled);
    void setPipSizeMode(const QString &mode);
    void setPipShowLandmarks(bool enabled);
    Q_INVOKABLE void setPipPosition(int x, int y);

    Q_INVOKABLE QVariantMap loadSettings() const;
    Q_INVOKABLE bool saveSettings(const QVariantMap &settings);

signals:
    void themeModeChanged();
    void effectiveThemeChanged();
    void pipEnabledChanged();
    void pipLayoutChanged();
    void saveErrorChanged();
    void shortcutsChanged();

private:
    void setSaveError(const QString &error);

    QString themeMode_{QStringLiteral("dark")};
    bool pipEnabled_{true};
    QString pipSizeMode_{QStringLiteral("medium")};
    bool pipShowLandmarks_{true};
    int pipX_{-1};
    int pipY_{-1};
    QString saveError_;
    QString shortcutTracking_{QStringLiteral("Ctrl+Alt+F9")};
    QString shortcutStop_{QStringLiteral("Ctrl+Alt+F10")};
    QString shortcutNextProfile_{QStringLiteral("Ctrl+Alt+F11")};
};

} // namespace vc
