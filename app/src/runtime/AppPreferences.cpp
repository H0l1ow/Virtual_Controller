#include "runtime/AppPreferences.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSettings>
#include <QStyleHints>
#include <QDebug>

namespace vc {
namespace {
constexpr auto kSettingsKey = "ui/settings_v1";

bool validPipSize(const QString &mode)
{
    return mode == QStringLiteral("small")
        || mode == QStringLiteral("medium")
        || mode == QStringLiteral("large");
}

bool validTheme(const QString &mode)
{
    return mode == QStringLiteral("dark")
        || mode == QStringLiteral("light")
        || mode == QStringLiteral("system");
}
} // namespace

AppPreferences::AppPreferences(QObject *parent)
    : QObject(parent)
{
    const QVariantMap saved = loadSettings();
    const QString savedTheme = saved.value(
        QStringLiteral("themeMode"), QStringLiteral("dark")).toString();
    if (validTheme(savedTheme))
        themeMode_ = savedTheme;

    pipEnabled_ = saved.value(QStringLiteral("pipEnabled"), true).toBool();
    const QString size = saved.value(QStringLiteral("pipSizeMode"),
        QStringLiteral("medium")).toString();
    if (validPipSize(size))
        pipSizeMode_ = size;
    pipShowLandmarks_ = saved.value(QStringLiteral("pipShowLandmarks"), true).toBool();
    pipX_ = saved.value(QStringLiteral("pipX"), -1).toInt();
    pipY_ = saved.value(QStringLiteral("pipY"), -1).toInt();
    pipScreenName_ = saved.value(QStringLiteral("pipScreenName")).toString();
    const QString tracking = saved.value(QStringLiteral("shortcutTracking"), shortcutTracking_).toString();
    const QString stop = saved.value(QStringLiteral("shortcutStop"), shortcutStop_).toString();
    const QString next = saved.value(QStringLiteral("shortcutNextProfile"), shortcutNextProfile_).toString();
    if (validShortcut(tracking)) shortcutTracking_ = tracking;
    if (validShortcut(stop)) shortcutStop_ = stop;
    if (validShortcut(next)) shortcutNextProfile_ = next;

    auto *guiApp = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    connect(guiApp, &QGuiApplication::screenAdded, this,
            [this](QScreen *) { emit pipLayoutChanged(); });
    connect(guiApp, &QGuiApplication::screenRemoved, this,
            [this](QScreen *) { emit pipLayoutChanged(); });
    connect(guiApp, &QGuiApplication::primaryScreenChanged, this,
            [this](QScreen *) { emit pipLayoutChanged(); });

    connect(QGuiApplication::styleHints(),
            &QStyleHints::colorSchemeChanged,
            this,
            [this](Qt::ColorScheme) {
                if (themeMode_ == QStringLiteral("system"))
                    emit effectiveThemeChanged();
            });
}

bool AppPreferences::validShortcut(const QString &shortcut)
{
    if (shortcut == QStringLiteral("Disabled")) return true;
    for (int key = 5; key <= 12; ++key) {
        if (key == 8) continue; // F8 is reserved for OutputGuard emergency stop.
        if (shortcut == QStringLiteral("Ctrl+Alt+F%1").arg(key)) return true;
    }
    return false;
}

void AppPreferences::setShortcutTracking(const QString &value)
{
    if (!validShortcut(value) || shortcutTracking_ == value) return;
    shortcutTracking_ = value;
    emit shortcutsChanged();
}

void AppPreferences::setShortcutStop(const QString &value)
{
    if (!validShortcut(value) || shortcutStop_ == value) return;
    shortcutStop_ = value;
    emit shortcutsChanged();
}

void AppPreferences::setShortcutNextProfile(const QString &value)
{
    if (!validShortcut(value) || shortcutNextProfile_ == value) return;
    shortcutNextProfile_ = value;
    emit shortcutsChanged();
}

QString AppPreferences::effectiveTheme() const
{
    if (themeMode_ != QStringLiteral("system"))
        return themeMode_;

    return QGuiApplication::styleHints()->colorScheme()
            == Qt::ColorScheme::Light
        ? QStringLiteral("light")
        : QStringLiteral("dark");
}

void AppPreferences::setThemeMode(const QString &mode)
{
    if (!validTheme(mode) || themeMode_ == mode)
        return;

    themeMode_ = mode;
    emit themeModeChanged();
    emit effectiveThemeChanged();
}

void AppPreferences::setPipEnabled(bool enabled)
{
    if (pipEnabled_ == enabled)
        return;

    pipEnabled_ = enabled;
    emit pipEnabledChanged();
}

void AppPreferences::setPipSizeMode(const QString &mode)
{
    if (!validPipSize(mode) || pipSizeMode_ == mode)
        return;
    pipSizeMode_ = mode;
    emit pipLayoutChanged();
}

void AppPreferences::setPipShowLandmarks(bool enabled)
{
    if (pipShowLandmarks_ == enabled)
        return;
    pipShowLandmarks_ = enabled;
    emit pipLayoutChanged();
}

QStringList AppPreferences::pipScreenNames() const
{
    QStringList names;
    for (auto *screen : QGuiApplication::screens())
        names.append(screen->name());
    return names;
}

QScreen *AppPreferences::pipScreen() const
{
    for (auto *screen : QGuiApplication::screens()) {
        if (!pipScreenName_.isEmpty() && screen->name() == pipScreenName_)
            return screen;
    }
    return QGuiApplication::primaryScreen();
}

void AppPreferences::setPipScreenName(const QString &name)
{
    if (!name.isEmpty() && !pipScreenNames().contains(name))
        return;
    if (pipScreenName_ == name)
        return;
    pipScreenName_ = name;
    pipX_ = -1;
    pipY_ = -1;
    emit pipLayoutChanged();
}

void AppPreferences::setPipPosition(int x, int y)
{
    // (-1,-1) is the documented bottom-right default; other negative
    // positions are valid on monitors positioned left/above primary.
    if (x == -1 && y == -1) {
        x = -1;
        y = -1;
    }
    if (pipX_ == x && pipY_ == y)
        return;
    pipX_ = x;
    pipY_ = y;
    emit pipLayoutChanged();
}

QVariantMap AppPreferences::loadSettings() const
{
    const QSettings settings;
    const QByteArray json = settings.value(
        QString::fromLatin1(kSettingsKey)).toByteArray();
    if (json.isEmpty())
        return {};

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "[settings] Invalid preferences, using defaults:" << error.errorString();
        return {};
    }
    return document.toVariant().toMap();
}

bool AppPreferences::saveSettings(const QVariantMap &values)
{
    QVariantMap snapshot = values;
    // Persist only validated values. UI previews the changes until Save.
    snapshot.insert(QStringLiteral("themeMode"), themeMode_);
    snapshot.insert(QStringLiteral("pipEnabled"), pipEnabled_);
    snapshot.insert(QStringLiteral("pipSizeMode"), pipSizeMode_);
    snapshot.insert(QStringLiteral("pipShowLandmarks"), pipShowLandmarks_);
    snapshot.insert(QStringLiteral("pipX"), pipX_);
    snapshot.insert(QStringLiteral("pipY"), pipY_);
    snapshot.insert(QStringLiteral("pipScreenName"), pipScreenName_);
    snapshot.insert(QStringLiteral("shortcutTracking"), shortcutTracking_);
    snapshot.insert(QStringLiteral("shortcutStop"), shortcutStop_);
    snapshot.insert(QStringLiteral("shortcutNextProfile"), shortcutNextProfile_);

    const QJsonDocument document = QJsonDocument::fromVariant(snapshot);
    if (document.isNull() || !document.isObject()) {
        setSaveError(QStringLiteral("Could not serialize settings."));
        return false;
    }

    QSettings settings;
    settings.setValue(QString::fromLatin1(kSettingsKey),
                      document.toJson(QJsonDocument::Compact));
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        setSaveError(QStringLiteral("Could not write settings to the user profile."));
        return false;
    }

    setSaveError(QString{});
    return true;
}

void AppPreferences::setSaveError(const QString &error)
{
    if (saveError_ == error)
        return;
    saveError_ = error;
    emit saveErrorChanged();
}

} // namespace vc
