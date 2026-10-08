#include "mapping/ProfileStore.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <algorithm>
#include <utility>

namespace vc {

namespace {

QString handName(HandSide hand)
{
    return hand == HandSide::Left
        ? QStringLiteral("Left")
        : QStringLiteral("Right");
}

QString gestureName(GestureClass gesture)
{
    return QString::fromLatin1(gestureCanonicalName(gesture));
}

QString actionName(LogicalAction action)
{
    return QString::fromLatin1(logicalActionId(action));
}

QString behaviorName(ActionBehavior behavior)
{
    return QString::fromLatin1(actionBehaviorName(behavior));
}

bool parseHand(const QString &value, HandSide *out)
{
    if (value.compare(QStringLiteral("Left"), Qt::CaseInsensitive) == 0) {
        *out = HandSide::Left;
        return true;
    }
    if (value.compare(QStringLiteral("Right"), Qt::CaseInsensitive) == 0) {
        *out = HandSide::Right;
        return true;
    }
    return false;
}

bool parseGesture(const QString &value, GestureClass *out)
{
    for (std::size_t index = 0; index < kGestureCanonicalNames.size(); ++index) {
        if (value.compare(
                QString::fromLatin1(kGestureCanonicalNames[index]),
                Qt::CaseInsensitive) == 0) {
            *out = static_cast<GestureClass>(index);
            return true;
        }
    }
    return false;
}

bool parseAction(const QString &value, LogicalAction *out)
{
    for (int index = static_cast<int>(LogicalAction::MouseLeft);
         index <= static_cast<int>(LogicalAction::KeyAlt);
         ++index) {
        const auto action = static_cast<LogicalAction>(index);
        if (value.compare(
                QString::fromLatin1(logicalActionId(action)),
                Qt::CaseInsensitive) == 0) {
            *out = action;
            return true;
        }
    }
    return false;
}

bool parseBehavior(const QString &value, ActionBehavior *out)
{
    if (value.compare(QStringLiteral("Press"), Qt::CaseInsensitive) == 0) {
        *out = ActionBehavior::Press;
        return true;
    }
    if (value.compare(QStringLiteral("Hold"), Qt::CaseInsensitive) == 0) {
        *out = ActionBehavior::Hold;
        return true;
    }
    if (value.compare(QStringLiteral("Toggle"), Qt::CaseInsensitive) == 0) {
        *out = ActionBehavior::Toggle;
        return true;
    }
    return false;
}

} // namespace

ProfileStore::ProfileStore(
    QString bundledDirectory,
    QString writableDirectory)
    : bundledDirectory_(std::move(bundledDirectory))
    , writableDirectory_(std::move(writableDirectory))
{
}

bool ProfileStore::initialize(QString *error)
{
    QDir writable(writableDirectory_);
    if (!writable.exists() && !writable.mkpath(QStringLiteral("."))) {
        if (error) {
            *error = QStringLiteral("Could not create profile directory: %1")
                .arg(writableDirectory_);
        }
        return false;
    }

    QDir bundled(bundledDirectory_);
    if (!bundled.exists()) {
        return true;
    }

    const QStringList files = bundled.entryList(
        {QStringLiteral("*.json")},
        QDir::Files,
        QDir::Name);

    for (const auto &fileName : files) {
        const QString destination = writable.filePath(fileName);
        if (QFile::exists(destination)) {
            continue;
        }
        if (!QFile::copy(bundled.filePath(fileName), destination)) {
            if (error) {
                *error = QStringLiteral("Could not seed profile: %1")
                    .arg(fileName);
            }
            return false;
        }
    }

    return true;
}

QStringList ProfileStore::profileNames() const
{
    QDir directory(writableDirectory_);
    QStringList result;
    for (const auto &fileName : directory.entryList(
             {QStringLiteral("*.json")},
             QDir::Files,
             QDir::Name)) {
        result.append(QFileInfo(fileName).completeBaseName());
    }
    return result;
}

bool ProfileStore::load(
    const QString &name,
    MappingProfile *profile,
    QString *error) const
{
    if (!profile) {
        if (error) *error = QStringLiteral("Profile output pointer is null.");
        return false;
    }

    QFile file(QDir(writableDirectory_).filePath(name + QStringLiteral(".json")));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Could not open profile: %1").arg(name);
        }
        return false;
    }

    return parseProfile(file.readAll(), profile, error);
}

bool ProfileStore::save(
    const MappingProfile &profile,
    QString *error) const
{
    const QString name = QString::fromStdString(profile.name).trimmed();
    if (name.isEmpty()) {
        if (error) *error = QStringLiteral("Profile name cannot be empty.");
        return false;
    }

    QSaveFile file(QDir(writableDirectory_).filePath(name + QStringLiteral(".json")));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("Could not write profile: %1").arg(name);
        }
        return false;
    }

    const QByteArray bytes = serializeProfile(profile);
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        if (error) *error = QStringLiteral("Could not fully write profile: %1").arg(name);
        return false;
    }

    if (!file.commit()) {
        if (error) *error = QStringLiteral("Could not atomically save profile: %1").arg(name);
        return false;
    }

    return true;
}

bool ProfileStore::parseProfile(
    const QByteArray &json,
    MappingProfile *profile,
    QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Invalid profile JSON: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schema")).toString()
        != QStringLiteral("vc.mapping.profile.v1")) {
        if (error) *error = QStringLiteral("Unsupported mapping profile schema.");
        return false;
    }

    MappingProfile parsed;
    parsed.name = root.value(QStringLiteral("name")).toString().toStdString();

    const QJsonArray mappings = root.value(QStringLiteral("mappings")).toArray();
    for (const auto &value : mappings) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        MappingRule rule;
        rule.id = object.value(QStringLiteral("id")).toString().toStdString();
        if (rule.id.empty()) {
            continue;
        }

        if (!parseHand(object.value(QStringLiteral("hand")).toString(), &rule.hand)
            || !parseGesture(object.value(QStringLiteral("gesture")).toString(), &rule.gesture)
            || !parseAction(object.value(QStringLiteral("action")).toString(), &rule.action)
            || !parseBehavior(object.value(QStringLiteral("behavior")).toString(), &rule.behavior)) {
            continue;
        }

        rule.enabled = object.value(QStringLiteral("enabled")).toBool(true);

        // Cursor movement activation is stateful. A one-frame Press gate is
        // not useful and could make hand movement appear broken, so hand-edited
        // legacy/custom JSON is normalized to Hold. The editor itself only
        // allows Hold or Toggle for this action.
        if (rule.action == LogicalAction::CursorMoveEnable
            && rule.behavior == ActionBehavior::Press) {
            rule.behavior = ActionBehavior::Hold;
        }

        // 0.9.1 accidentally seeded this mapping in the bundled Default
        // profile. Treat only that exact legacy seed ID as obsolete so users
        // upgrading from 0.9.1 do not keep a permanent FIST -> Freeze row.
        // A user-created FIST -> Freeze mapping uses a custom_* ID and remains
        // fully supported.
        if (rule.id == "right_fist_cursor_freeze"
            && rule.hand == HandSide::Right
            && rule.gesture == GestureClass::Fist
            && rule.action == LogicalAction::CursorFreeze
            && rule.behavior == ActionBehavior::Hold) {
            continue;
        }

        parsed.mappings.push_back(rule);
    }

    if (parsed.name.empty()) {
        if (error) *error = QStringLiteral("Profile name is missing.");
        return false;
    }

    *profile = std::move(parsed);
    return true;
}

QByteArray ProfileStore::serializeProfile(
    const MappingProfile &profile)
{
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("vc.mapping.profile.v1"));
    root.insert(QStringLiteral("name"), QString::fromStdString(profile.name));

    QJsonArray mappings;
    for (const auto &rule : profile.mappings) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), QString::fromStdString(rule.id));
        object.insert(QStringLiteral("hand"), handName(rule.hand));
        object.insert(QStringLiteral("gesture"), gestureName(rule.gesture));
        object.insert(QStringLiteral("action"), actionName(rule.action));
        object.insert(QStringLiteral("behavior"), behaviorName(rule.behavior));
        object.insert(QStringLiteral("enabled"), rule.enabled);
        mappings.append(object);
    }
    root.insert(QStringLiteral("mappings"), mappings);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

} // namespace vc
