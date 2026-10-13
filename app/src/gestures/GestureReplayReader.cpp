#include "gestures/GestureReplayReader.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrl>

#include <cmath>

namespace vc {

bool GestureReplayReader::open(const QString &filePath)
{
    close();
    error_.clear();
    const QUrl url(filePath);
    const QString path = url.isLocalFile() ? url.toLocalFile() : filePath;
    if (!path.endsWith(QStringLiteral(".jsonl"), Qt::CaseInsensitive)) {
        error_ = QStringLiteral("Select a .jsonl landmark recording.");
        return false;
    }
    file_.setFileName(path);
    if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error_ = file_.errorString();
        return false;
    }
    if (file_.size() == 0 || file_.size() > 100 * 1024 * 1024) {
        error_ = QStringLiteral("Recording is empty or exceeds 100 MiB.");
        close();
        return false;
    }
    framesRead_ = 0;
    lastTimestamp_ = 0;
    finished_ = false;
    return true;
}

bool GestureReplayReader::next(
    TrackingFrame *frame, std::array<QString, kHandCount> *labels)
{
    if (!active() || !frame || !labels || finished_)
        return false;
    if (file_.atEnd()) {
        finished_ = true;
        return false;
    }
    if (framesRead_ >= 18000) {
        error_ = QStringLiteral("Recording exceeds the 18000-frame limit.");
        finished_ = true;
        return false;
    }
    const QByteArray line = file_.readLine(32769);
    if (line.size() > 32768 || !line.endsWith('\n') && !file_.atEnd()) {
        error_ = QStringLiteral("Recording contains an oversized or incomplete line.");
        finished_ = true;
        return false;
    }
    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error_ = QStringLiteral("Invalid JSONL at frame %1.").arg(framesRead_ + 1);
        finished_ = true;
        return false;
    }
    const auto object = document.object();
    const auto hands = object.value(QStringLiteral("hands")).toArray();
    const auto timestamp = object.value(QStringLiteral("timestamp_us")).toInteger();
    const auto width = object.value(QStringLiteral("width")).toInt();
    const auto height = object.value(QStringLiteral("height")).toInt();
    if (object.value(QStringLiteral("schema")).toString() != QStringLiteral("vc.gestures.jsonl.v1")
        || hands.size() != 2 || timestamp <= lastTimestamp_ || width <= 0 || height <= 0) {
        error_ = QStringLiteral("Invalid tracking frame or timestamp at line %1.").arg(framesRead_ + 1);
        finished_ = true;
        return false;
    }
    TrackingFrame nextFrame{};
    nextFrame.width = width;
    nextFrame.height = height;
    nextFrame.captureUs = timestamp;
    nextFrame.sequence = static_cast<std::uint64_t>(object.value(QStringLiteral("sequence")).toInteger());
    std::array<QString, kHandCount> nextLabels{};
    for (int i = 0; i < 2; ++i) {
        const auto objectHand = hands.at(i).toObject();
        const auto sideName = objectHand.value(QStringLiteral("side")).toString();
        if (sideName != (i == 0 ? QStringLiteral("LEFT") : QStringLiteral("RIGHT"))) {
            error_ = QStringLiteral("Unexpected hand order in recording.");
            finished_ = true;
            return false;
        }
        auto &hand = nextFrame.hands[static_cast<std::size_t>(i)];
        hand.side = i == 0 ? HandSide::Left : HandSide::Right;
        hand.reportedSide = objectHand.value(QStringLiteral("reported_side")).toString()
                == QStringLiteral("RIGHT") ? HandSide::Right : HandSide::Left;
        hand.tracked = objectHand.value(QStringLiteral("tracked")).toBool();
        hand.trackId = static_cast<std::uint64_t>(objectHand.value(QStringLiteral("track_id")).toInteger());
        hand.handednessConfidence = static_cast<float>(objectHand.value(
            QStringLiteral("handedness_confidence")).toDouble());
        nextLabels[static_cast<std::size_t>(i)] = objectHand.value(QStringLiteral("label")).toString();
        if (!hand.tracked)
            continue;
        const QJsonArray points = objectHand.value(QStringLiteral("landmarks")).toArray();
        if (hand.trackId == 0 || points.size() != static_cast<int>(kHandLandmarkCount)) {
            error_ = QStringLiteral("Invalid tracked hand landmarks.");
            finished_ = true;
            return false;
        }
        for (std::size_t n = 0; n < kHandLandmarkCount; ++n) {
            const QJsonArray xyz = points.at(static_cast<int>(n)).toArray();
            if (xyz.size() != 3) {
                error_ = QStringLiteral("Invalid landmark shape.");
                finished_ = true;
                return false;
            }
            float values[3];
            for (int axis = 0; axis < 3; ++axis) {
                if (!xyz.at(axis).isDouble() || !std::isfinite(xyz.at(axis).toDouble())) {
                    error_ = QStringLiteral("Nonfinite landmark in recording.");
                    finished_ = true;
                    return false;
                }
                values[axis] = static_cast<float>(xyz.at(axis).toDouble());
            }
            hand.landmarks[n] = {values[0], values[1], values[2]};
        }
    }
    *frame = nextFrame;
    *labels = nextLabels;
    lastTimestamp_ = timestamp;
    ++framesRead_;
    return true;
}

void GestureReplayReader::close()
{
    file_.close();
    finished_ = true;
    framesRead_ = 0;
    lastTimestamp_ = 0;
}

} // namespace vc
