#pragma once

#include "mapping/MappingTypes.hpp"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace vc {

class ProfileStore final
{
public:
    ProfileStore(
        QString bundledDirectory,
        QString writableDirectory);

    bool initialize(QString *error = nullptr);
    QStringList profileNames() const;

    bool load(
        const QString &name,
        MappingProfile *profile,
        QString *error = nullptr) const;

    bool save(
        const MappingProfile &profile,
        QString *error = nullptr) const;

    QString writableDirectory() const { return writableDirectory_; }

private:
    static bool parseProfile(
        const QByteArray &json,
        MappingProfile *profile,
        QString *error);

    static QByteArray serializeProfile(
        const MappingProfile &profile);

    QString bundledDirectory_;
    QString writableDirectory_;
};

} // namespace vc
