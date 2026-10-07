#include "tracking/MediaPipeTracker.hpp"

#include <QFile>
#include <QDebug>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace vc {

namespace {

template<typename T>
T resolveSymbol(
    QLibrary &library,
    const char *name)
{
    const auto symbol =
        library.resolve(name);

    if (!symbol) {
        throw std::runtime_error(
            std::string(
                "MediaPipe 0.10.32: missing symbol ")
            + name);
    }

    return reinterpret_cast<T>(symbol);
}

} // namespace

void MediaPipeTracker::check(
    int code,
    char *error)
{
    const std::string message =
        error
        ? error
        : "MediaPipe native API error";

    if (error) {
        errorFree_(error);
    }

    if (code != 0) {
        throw std::runtime_error(message);
    }
}

MediaPipeTracker::MediaPipeTracker(
    const QString &libraryPath,
    const QString &modelPath,
    float detectionConfidence,
    float trackingConfidence)
    : library_(libraryPath)
{
    // MediaPipe/TFLite owns process-wide native state and worker pools.
    // Repeated FreeLibrary()/LoadLibrary() cycles are unnecessary and can be
    // fragile on Windows. Keep the runtime mapped until process termination.
    library_.setLoadHints(
        library_.loadHints()
        | QLibrary::PreventUnloadHint);

    if (!library_.load()) {
        throw std::runtime_error(
            ("Cannot load MediaPipe runtime: "
             + library_.errorString())
                .toStdString());
    }

    create_ =
        resolveSymbol<mp::Create>(
            library_,
            "MpHandLandmarkerCreate");

    detect_ =
        resolveSymbol<mp::Detect>(
            library_,
            "MpHandLandmarkerDetectForVideo");

    closeResult_ =
        resolveSymbol<mp::CloseResult>(
            library_,
            "MpHandLandmarkerCloseResult");

    close_ =
        resolveSymbol<mp::Close>(
            library_,
            "MpHandLandmarkerClose");

    imageCreate_ =
        resolveSymbol<mp::ImageCreate>(
            library_,
            "MpImageCreateFromUint8Data");

    imageFree_ =
        resolveSymbol<mp::ImageFree>(
            library_,
            "MpImageFree");

    errorFree_ =
        resolveSymbol<mp::ErrorFree>(
            library_,
            "MpErrorFree");

    QFile modelFile(modelPath);

    if (!modelFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(
            "Cannot open hand_landmarker.task");
    }

    const QByteArray bytes =
        modelFile.readAll();

    if (bytes.size() < 1024
        || bytes.size() > 100000000) {
        throw std::runtime_error(
            "Invalid hand_landmarker.task size");
    }

    model_.assign(
        bytes.begin(),
        bytes.end());

    mp::Options options{};

    options.base.buffer =
        model_.data();

    options.base.count =
        static_cast<unsigned int>(
            model_.size());

    options.base.delegate = 0;

    // MediaPipe RunningMode::VIDEO.
    options.runningMode = 2;

    // M1 requirement: both hands from the start.
    options.numHands = 2;

    options.detection =
        std::clamp(
            detectionConfidence,
            0.0F,
            1.0F);

    // M1 does not expose a separate presence threshold.
    options.presence =
        options.detection;

    options.tracking =
        std::clamp(
            trackingConfidence,
            0.0F,
            1.0F);

    char *error = nullptr;

    const int code =
        create_(
            &options,
            &handle_,
            &error);

    check(code, error);
}

MediaPipeTracker::~MediaPipeTracker()
{
    if (!handle_) {
        return;
    }

    qInfo() << "[shutdown] MpHandLandmarkerClose begin";

    char *error = nullptr;

    close_(
        handle_,
        &error);

    handle_ = nullptr;

    if (error) {
        errorFree_(error);
    }

    qInfo() << "[shutdown] MpHandLandmarkerClose end";
}

RawTrackingFrame MediaPipeTracker::process(
    int width,
    int height,
    const std::vector<std::uint8_t> &rgb,
    std::int64_t captureUs,
    std::uint64_t sequence)
{
    const auto expectedSize =
        static_cast<std::size_t>(width)
        * static_cast<std::size_t>(height)
        * 3U;

    if (width <= 0
        || height <= 0
        || rgb.size() != expectedSize) {
        throw std::runtime_error(
            "Invalid RGB888 camera frame");
    }

    RawTrackingFrame output;

    output.width = width;
    output.height = height;
    output.captureUs = captureUs;
    output.sequence = sequence;
    output.trackStartUs = nowUs();

    mp::Image image = nullptr;
    char *error = nullptr;

    int code =
        imageCreate_(
            1,
            width,
            height,
            rgb.data(),
            static_cast<int>(rgb.size()),
            &image,
            &error);

    check(code, error);

    mp::Result result{};

    try {
        const auto timestampMs =
            std::max(
                lastTimestampMs_ + 1,
                captureUs / 1000);

        lastTimestampMs_ =
            timestampMs;

        error = nullptr;

        code =
            detect_(
                handle_,
                image,
                nullptr,
                timestampMs,
                &result,
                &error);

        check(code, error);

        const auto handCount =
            std::min(
                result.landmarksCount,
                result.handednessCount);

        std::size_t outputIndex = 0;

        for (std::uint32_t handIndex = 0;
             handIndex < handCount
             && outputIndex < kHandCount;
             ++handIndex) {

            if (result.landmarks[handIndex].count
                    != static_cast<std::uint32_t>(
                        kHandLandmarkCount)
                || result.handedness[handIndex].count == 0) {
                continue;
            }

            const auto &category =
                result.handedness[handIndex].items[0];

            if (!category.name) {
                continue;
            }

            const std::string handedness(
                category.name);

            if (handedness != "Left"
                && handedness != "Right") {
                continue;
            }

            auto &detection =
                output.detections[outputIndex++];

            detection.detected = true;
            detection.reportedSide =
                handedness == "Left"
                ? HandSide::Left
                : HandSide::Right;
            detection.handednessConfidence =
                category.score;

            for (std::size_t landmarkIndex = 0;
                 landmarkIndex < kHandLandmarkCount;
                 ++landmarkIndex) {

                const auto &point =
                    result
                        .landmarks[handIndex]
                        .points[landmarkIndex];

                detection.landmarks[landmarkIndex] = {
                    point.x,
                    point.y,
                    point.z
                };
            }
        }
    }
    catch (...) {
        closeResult_(&result);
        imageFree_(image);
        throw;
    }

    closeResult_(&result);
    imageFree_(image);

    output.trackEndUs =
        nowUs();

    return output;
}

} // namespace vc
