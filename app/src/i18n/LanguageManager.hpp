#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QTranslator>

class QQmlEngine;

namespace vc {

// Qt's qsTr() / QQmlEngine::retranslate() path, with JSON catalogs bundled as
// Qt resources. No external Qt Linguist tool or runtime file is required.
class JsonTranslator final : public QTranslator {
public:
    bool loadCatalog(const QString &resourcePath);
    QString translate(const char *context,
                      const char *sourceText,
                      const char *disambiguation = nullptr,
                      int n = -1) const override;

private:
    QHash<QString, QString> m_messages;
};

class LanguageManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(int languageIndex READ languageIndex NOTIFY languageChanged)
    Q_PROPERTY(QStringList languageNames READ languageNames CONSTANT)

public:
    explicit LanguageManager(QQmlEngine *engine, QObject *parent = nullptr);
    ~LanguageManager() override;

    [[nodiscard]] QString language() const;
    [[nodiscard]] int languageIndex() const;
    [[nodiscard]] QStringList languageNames() const;

    Q_INVOKABLE bool setLanguage(const QString &code);
    Q_INVOKABLE bool setLanguageByIndex(int index);

signals:
    void languageChanged();

private:
    QQmlEngine *m_engine = nullptr;
    JsonTranslator m_translator;
    QString m_language = QStringLiteral("en");
};

} // namespace vc
