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
    Q_PROPERTY(QString saveError READ saveError NOTIFY saveErrorChanged)

public:
    explicit AppPreferences(QObject *parent = nullptr);

    QString themeMode() const { return themeMode_; }
    QString effectiveTheme() const;
    bool pipEnabled() const { return pipEnabled_; }
    QString saveError() const { return saveError_; }

    void setThemeMode(const QString &mode);
    void setPipEnabled(bool enabled);

    Q_INVOKABLE QVariantMap loadSettings() const;
    Q_INVOKABLE bool saveSettings(const QVariantMap &settings);

signals:
    void themeModeChanged();
    void effectiveThemeChanged();
    void pipEnabledChanged();
    void saveErrorChanged();

private:
    void setSaveError(const QString &error);

    QString themeMode_{QStringLiteral("dark")};
    bool pipEnabled_{true};
    QString saveError_;
};

} // namespace vc
