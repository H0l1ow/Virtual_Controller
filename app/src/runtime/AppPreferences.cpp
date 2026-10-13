#include "runtime/AppPreferences.hpp"

#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSettings>
#include <QStyleHints>
#include <QDebug>

namespace vc {
namespace {
constexpr auto kSettingsKey = "ui/settings_v1";

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

    connect(QGuiApplication::styleHints(),
            &QStyleHints::colorSchemeChanged,
            this,
            [this](Qt::ColorScheme) {
                if (themeMode_ == QStringLiteral("system"))
                    emit effectiveThemeChanged();
            });
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
