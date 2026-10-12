#include "LanguageManager.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QList>
#include <QQmlEngine>
#include <QSettings>
#include <QDebug>

namespace vc {

namespace {

struct SupportedLanguage {
    QString code;
    QString nativeName;
};

const QList<SupportedLanguage> &supportedLanguages()
{
    // To add another language: extend this list and add i18n/<code>.json
    // to app/CMakeLists.txt. The selector updates automatically.
    static const QList<SupportedLanguage> languages{
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("pl"), QStringLiteral("Polski")},
    };
    return languages;
}

} // namespace

bool JsonTranslator::loadCatalog(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[i18n] Cannot open catalog:" << resourcePath;
        return false;
    }

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "[i18n] Invalid translation catalog:"
                   << resourcePath << error.errorString();
        return false;
    }

    // Swap only after successful parsing, so a bad future catalog cannot
    // invalidate the currently selected language.
    QHash<QString, QString> messages;
    const QJsonObject object = document.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.value().isString() && !it.value().toString().isEmpty()) {
            messages.insert(it.key(), it.value().toString());
        }
    }
    if (messages.isEmpty()) {
        qWarning() << "[i18n] Empty catalog:" << resourcePath;
        return false;
    }
    m_messages.swap(messages);
    return true;
}

QString JsonTranslator::translate(const char *context,
                                  const char *sourceText,
                                  const char *disambiguation,
                                  int n) const
{
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);
    if (!sourceText)
        return {};

    // The same label should have the same UI translation in every component.
    // Returning an empty QString lets Qt use the original English text.
    auto result = m_messages.value(QString::fromUtf8(sourceText));
    if (n >= 0 && !result.isEmpty())
        result.replace(QStringLiteral("%n"), QString::number(n));
    return result;
}

LanguageManager::LanguageManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    const auto stored = QSettings().value(QStringLiteral("ui/language"),
                                          QStringLiteral("en")).toString();
    if (!setLanguage(stored))
        setLanguage(QStringLiteral("en"));
}

LanguageManager::~LanguageManager()
{
    if (m_language != QStringLiteral("en"))
        QCoreApplication::removeTranslator(&m_translator);
}

QString LanguageManager::language() const
{
    return m_language;
}

int LanguageManager::languageIndex() const
{
    const auto &languages = supportedLanguages();
    for (int i = 0; i < languages.size(); ++i) {
        if (languages[i].code == m_language)
            return i;
    }
    return 0;
}

QStringList LanguageManager::languageNames() const
{
    // Native language names remain recognizable regardless of UI locale.
    QStringList names;
    for (const auto &entry : supportedLanguages())
        names.append(entry.nativeName);
    return names;
}

bool LanguageManager::setLanguageByIndex(int index)
{
    const auto &languages = supportedLanguages();
    return index >= 0 && index < languages.size()
        && setLanguage(languages[index].code);
}

bool LanguageManager::setLanguage(const QString &code)
{
    bool supported = false;
    for (const auto &entry : supportedLanguages()) {
        if (entry.code == code) {
            supported = true;
            break;
        }
    }
    if (!supported)
        return false;

    if (code == m_language)
        return true;

    if (code != QStringLiteral("en")) {
        if (!m_translator.loadCatalog(QStringLiteral(":/i18n/") + code
                                      + QStringLiteral(".json")))
            return false;
        // Moving between two non-English locales also works: one translator
        // object is detached and then reinstalled after its catalog changes.
        QCoreApplication::removeTranslator(&m_translator);
        if (!QCoreApplication::installTranslator(&m_translator))
            return false;
    } else {
        QCoreApplication::removeTranslator(&m_translator);
    }

    m_language = code;
    QSettings().setValue(QStringLiteral("ui/language"), m_language);
    emit languageChanged();

    // Re-evaluate all qsTr(...) expressions without destroying the current
    // page, changing tracking state or reloading mapping profiles.
    if (m_engine)
        m_engine->retranslate();

    return true;
}

} // namespace vc
