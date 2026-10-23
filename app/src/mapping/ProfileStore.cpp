#include "mapping/ProfileStore.hpp"
#include "gestures/GestureRegistry.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QJsonValue>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
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

bool ProfileStore::validName(const QString &name)
{
    // No path separators, dot segments, reserved Windows filename characters,
    // or huge filenames. Unicode letters/numbers are supported.
    static const QRegularExpression pattern(
        QStringLiteral(R"(\A[\p{L}\p{N}][\p{L}\p{N} ._()-]{0,63}\z)"));
    static const QRegularExpression reserved(
        QStringLiteral(R"(\A(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\..*)?\z)"),
        QRegularExpression::CaseInsensitiveOption);
    return pattern.match(name).hasMatch() && name == name.trimmed()
        && !name.endsWith(QLatin1Char('.')) && !name.contains(QStringLiteral(".."))
        && !reserved.match(name).hasMatch();
}

bool ProfileStore::remove(const QString &name, QString *error) const
{
    if (!validName(name)) {
        if (error) *error = QStringLiteral("Invalid profile name.");
        return false;
    }
    if (!QFile::remove(QDir(writableDirectory_).filePath(name + QStringLiteral(".json")))) {
        if (error) *error = QStringLiteral("Could not delete profile: %1").arg(name);
        return false;
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
        const QString name = QFileInfo(fileName).completeBaseName();
        if (validName(name))
            result.append(name);
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

    if (!validName(name)) {
        if (error) *error = QStringLiteral("Invalid profile name.");
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
    if (!validName(name)) {
        if (error) *error = QStringLiteral("Invalid profile name (max 64 safe filename characters).");
        return false;
    }

    const QString path = QDir(writableDirectory_).filePath(name + QStringLiteral(".json"));
    // v1 is backed up on its first explicit v2 save, never silently replaced
    // during loading and never overwritten during later saves.
    if (QFile::exists(path)) {
        QFile previous(path);
        if (!previous.open(QIODevice::ReadOnly)) {
            if (error) *error = QStringLiteral("Could not back up existing profile.");
            return false;
        }
        QJsonParseError previousError;
        const auto oldDoc = QJsonDocument::fromJson(previous.readAll(), &previousError);
        if (previousError.error != QJsonParseError::NoError || !oldDoc.isObject()) {
            if (error) *error = QStringLiteral("Existing profile is invalid; refusing to overwrite it.");
            return false;
        }
        const auto oldSchema = oldDoc.object().value(QStringLiteral("schema")).toString();
        if (oldSchema != QStringLiteral("vc.mapping.profile.v1")
            && oldSchema != QStringLiteral("vc.mapping.profile.v2")) {
            if (error) *error = QStringLiteral("Existing profile has an unsupported schema.");
            return false;
        }
        if (oldSchema == QStringLiteral("vc.mapping.profile.v1")) {
            const QString backup = path + QStringLiteral(".v1.bak");
            if (!QFile::exists(backup) && !QFile::copy(path, backup)) {
                if (error) *error = QStringLiteral("Could not create v1 profile backup.");
                return false;
            }
        }
    }

    QSaveFile file(path);
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
    if (!profile) {
        if (error) *error = QStringLiteral("Profile output pointer is null.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Invalid profile JSON: %1")
            .arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = document.object();
    const auto schema = root.value(QStringLiteral("schema")).toString();
    const bool legacy = schema == QStringLiteral("vc.mapping.profile.v1");
    if (!legacy && schema != QStringLiteral("vc.mapping.profile.v2")) {
        if (error) *error = QStringLiteral("Unsupported mapping profile schema.");
        return false;
    }

    auto allowedKeys = [](const QJsonObject &object, const QStringList &allowed) {
        for (const QString &key : object.keys()) {
            if (!allowed.contains(key))
                return false;
        }
        return true;
    };
    if (!legacy && (!allowedKeys(root, {QStringLiteral("schema"),
                              QStringLiteral("name"), QStringLiteral("mappings")})
                    || !root.value(QStringLiteral("name")).isString()
                    || !root.value(QStringLiteral("mappings")).isArray())) {
        if (error) *error = QStringLiteral("Invalid v2 profile fields.");
        return false;
    }

    MappingProfile parsed;
    parsed.name = root.value(QStringLiteral("name")).toString().toStdString();
    if (parsed.name.empty()) {
        if (error) *error = QStringLiteral("Profile name is missing.");
        return false;
    }
    const QJsonArray mappings = root.value(QStringLiteral("mappings")).toArray();
    int mappingIndex = -1;
    for (const auto &value : mappings) {
        ++mappingIndex;
        if (!value.isObject()) {
            if (legacy) continue;
            if (error) *error = QStringLiteral("Invalid mapping #%1: not an object.")
                .arg(mappingIndex + 1);
            return false;
        }
        const QJsonObject object = value.toObject();
        MappingRule rule;
        rule.id = object.value(QStringLiteral("id")).toString().toStdString();
        if (rule.id.empty()) {
            if (legacy) continue;
            if (error) *error = QStringLiteral("Mapping #%1 is missing an ID.")
                .arg(mappingIndex + 1);
            return false;
        }
        bool valid = parseAction(object.value(QStringLiteral("action")).toString(), &rule.action)
            && parseBehavior(object.value(QStringLiteral("behavior")).toString(), &rule.behavior);
        if (legacy) {
            valid = valid
                && parseHand(object.value(QStringLiteral("hand")).toString(), &rule.hand)
                && parseGesture(object.value(QStringLiteral("gesture")).toString(), &rule.gesture);
        } else {
            valid = valid && allowedKeys(object, {QStringLiteral("id"),
                QStringLiteral("trigger"), QStringLiteral("action"),
                QStringLiteral("behavior"), QStringLiteral("enabled")})
                && object.value(QStringLiteral("enabled")).isBool()
                && object.value(QStringLiteral("trigger")).isObject();
            if (valid) {
                const auto trigger = object.value(QStringLiteral("trigger")).toObject();
                const QString type = trigger.value(QStringLiteral("type")).toString();
                if (type == QStringLiteral("single")) {
                    rule.triggerKind = TriggerKind::Single;
                    valid = allowedKeys(trigger, {QStringLiteral("type"),
                                QStringLiteral("hand"), QStringLiteral("gesture")})
                        && parseGesture(trigger.value(QStringLiteral("gesture")).toString(),
                                        &rule.gesture);
                    const QString hand = trigger.value(QStringLiteral("hand")).toString();
                    rule.anyHand = hand.compare(QStringLiteral("Any"), Qt::CaseInsensitive) == 0;
                    if (!rule.anyHand)
                        valid = valid && parseHand(hand, &rule.hand);
                } else if (type == QStringLiteral("two_hand_combo")) {
                    rule.triggerKind = TriggerKind::TwoHandCombination;
                    const auto sync = trigger.value(QStringLiteral("synchronizationMs"));
                    const auto overlap = trigger.value(QStringLiteral("overlapHoldMs"));
                    valid = allowedKeys(trigger, {QStringLiteral("type"),
                                QStringLiteral("left"), QStringLiteral("right"),
                                QStringLiteral("synchronizationMs"),
                                QStringLiteral("overlapHoldMs")})
                        && parseGesture(trigger.value(QStringLiteral("left")).toString(),
                                        &rule.gesture)
                        && parseGesture(trigger.value(QStringLiteral("right")).toString(),
                                        &rule.rightGesture)
                        && sync.isDouble() && overlap.isDouble()
                        && std::isfinite(sync.toDouble())
                        && std::isfinite(overlap.toDouble())
                        && sync.toDouble() >= 0 && sync.toDouble() <= 1500
                        && overlap.toDouble() >= 0 && overlap.toDouble() <= 1500
                        && std::floor(sync.toDouble()) == sync.toDouble()
                        && std::floor(overlap.toDouble()) == overlap.toDouble();
                    if (valid) {
                        rule.synchronizationMs = sync.toInt();
                        rule.overlapHoldMs = overlap.toInt();
                        valid = rule.synchronizationMs >= 0 && rule.synchronizationMs <= 1500
                            && rule.overlapHoldMs >= 0 && rule.overlapHoldMs <= 1500;
                    }
                } else {
                    valid = false;
                }
                // Unknown and planned classes are not silently discarded.
                // Keep the original JSON file untouched on validation failure.
                valid = valid && (isRecognizedGesture(rule.gesture)
                        || (rule.triggerKind == TriggerKind::Single
                            && rule.gesture == GestureClass::None))
                    && (rule.triggerKind != TriggerKind::TwoHandCombination
                        || isRecognizedGesture(rule.rightGesture));
            }
        }
        if (!valid || (rule.triggerKind == TriggerKind::TwoHandCombination
                       && rule.gesture == GestureClass::None)) {
            if (legacy) continue;
            if (error) *error = QStringLiteral("Invalid/unavailable trigger in mapping #%1 (%2).")
                .arg(mappingIndex + 1).arg(QString::fromStdString(rule.id));
            return false;
        }
        rule.enabled = object.value(QStringLiteral("enabled")).toBool(true);
        if (legacy && rule.action == LogicalAction::CursorMoveEnable
            && rule.behavior == ActionBehavior::Press)
            rule.behavior = ActionBehavior::Hold;
        // Only discard a known broken seed from historical v1 Default.
        if (legacy && rule.id == "right_fist_cursor_freeze"
            && rule.hand == HandSide::Right && rule.gesture == GestureClass::Fist
            && rule.action == LogicalAction::CursorFreeze
            && rule.behavior == ActionBehavior::Hold)
            continue;
        parsed.mappings.push_back(std::move(rule));
    }

    *profile = std::move(parsed); // never partially overwrite on failure
    return true;
}

QByteArray ProfileStore::serializeProfile(const MappingProfile &profile)
{
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("vc.mapping.profile.v2"));
    root.insert(QStringLiteral("name"), QString::fromStdString(profile.name));

    QJsonArray mappings;
    for (const auto &rule : profile.mappings) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), QString::fromStdString(rule.id));
        QJsonObject trigger;
        if (rule.triggerKind == TriggerKind::TwoHandCombination) {
            trigger.insert(QStringLiteral("type"), QStringLiteral("two_hand_combo"));
            trigger.insert(QStringLiteral("left"), gestureName(rule.gesture));
            trigger.insert(QStringLiteral("right"), gestureName(rule.rightGesture));
            trigger.insert(QStringLiteral("synchronizationMs"), rule.synchronizationMs);
            trigger.insert(QStringLiteral("overlapHoldMs"), rule.overlapHoldMs);
        } else {
            trigger.insert(QStringLiteral("type"), QStringLiteral("single"));
            trigger.insert(QStringLiteral("hand"), rule.anyHand
                ? QStringLiteral("Any") : handName(rule.hand));
            trigger.insert(QStringLiteral("gesture"), gestureName(rule.gesture));
        }
        object.insert(QStringLiteral("trigger"), trigger);
        object.insert(QStringLiteral("action"), actionName(rule.action));
        object.insert(QStringLiteral("behavior"), behaviorName(rule.behavior));
        object.insert(QStringLiteral("enabled"), rule.enabled);
        mappings.append(object);
    }
    root.insert(QStringLiteral("mappings"), mappings);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

} // namespace vc
