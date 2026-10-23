// M6.4.1 Qt-enabled profile tests; compiled only in GUI-capable Qt builds.
#include "mapping/ProfileStore.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cassert>

using namespace vc;

int main()
{
    QTemporaryDir dir;
    assert(dir.isValid());
    const QString bundled = dir.filePath("bundled");
    const QString user = dir.filePath("user");
    assert(QDir().mkpath(bundled));
    QFile sample(QDir(bundled).filePath("Default.json"));
    assert(sample.open(QIODevice::WriteOnly));
    const QByteArray legacy = R"({"schema":"vc.mapping.profile.v1","name":"Default","mappings":[
        {"id":"left_click","hand":"Right","gesture":"PINCH","action":"mouse.left","behavior":"Hold","enabled":true},
        {"id":"fist_key","hand":"Left","gesture":"FIST","action":"key.space","behavior":"Press","enabled":false}
    ]})";
    assert(sample.write(legacy) == legacy.size());
    sample.close();

    ProfileStore store(bundled, user);
    QString error;
    assert(store.initialize(&error));
    MappingProfile profile;
    assert(store.load("Default", &profile, &error));
    assert(profile.mappings.size() == 2);
    assert(profile.mappings[0].id == "left_click");
    assert(profile.mappings[0].triggerKind == TriggerKind::Single);
    assert(profile.mappings[0].hand == HandSide::Right);
    assert(profile.mappings[0].gesture == GestureClass::Pinch);
    assert(profile.mappings[1].enabled == false);

    MappingRule combo;
    combo.id = "both_fist";
    combo.triggerKind = TriggerKind::TwoHandCombination;
    combo.gesture = GestureClass::Fist;
    combo.rightGesture = GestureClass::Fist;
    combo.action = LogicalAction::KeyTab;
    combo.behavior = ActionBehavior::Press;
    combo.synchronizationMs = 285;
    combo.overlapHoldMs = 170;
    profile.mappings.push_back(combo);
    assert(store.save(profile, &error));
    QFile backup(QDir(user).filePath("Default.json.v1.bak"));
    assert(backup.open(QIODevice::ReadOnly));
    assert(backup.readAll() == legacy);
    backup.close();

    QFile current(QDir(user).filePath("Default.json"));
    assert(current.open(QIODevice::ReadOnly));
    const auto json = QJsonDocument::fromJson(current.readAll()).object();
    assert(json.value("schema").toString() == "vc.mapping.profile.v2");
    current.close();

    MappingProfile roundtrip;
    assert(store.load("Default", &roundtrip, &error));
    assert(roundtrip.mappings.size() == 3);
    assert(roundtrip.mappings[0].id == "left_click");
    assert(roundtrip.mappings[0].behavior == ActionBehavior::Hold);
    assert(roundtrip.mappings[2].triggerKind == TriggerKind::TwoHandCombination);
    assert(roundtrip.mappings[2].rightGesture == GestureClass::Fist);
    assert(roundtrip.mappings[2].synchronizationMs == 285);
    assert(roundtrip.mappings[2].overlapHoldMs == 170);

    // Future incompatible records are rejected without corrupting a loaded profile.
    QFile invalid(QDir(user).filePath("Default.json"));
    assert(invalid.open(QIODevice::WriteOnly | QIODevice::Truncate));
    invalid.write(R"({"schema":"vc.mapping.profile.v2","name":"Default","mappings":[
        {"id":"future","trigger":{"type":"single","hand":"Any","gesture":"THUMBS_UP"},
        "action":"key.space","behavior":"Press","enabled":false}]})");
    invalid.close();
    MappingProfile unchanged = roundtrip;
    assert(!store.load("Default", &unchanged, &error));
    assert(unchanged.mappings.size() == 3);
    assert(unchanged.mappings[2].id == "both_fist");
}
