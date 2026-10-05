#pragma once

#include "tracking/MediaPipeAbi.hpp"
#include "tracking/TrackingTypes.hpp"

#include <QLibrary>
#include <QString>

#include <cstdint>
#include <vector>

namespace vc {

class MediaPipeTracker final
{
public:
    MediaPipeTracker(
        const QString &libraryPath,
        const QString &modelPath,
        float detectionConfidence,
        float trackingConfidence);

    ~MediaPipeTracker();

    MediaPipeTracker(
        const MediaPipeTracker &) = delete;

    MediaPipeTracker &operator=(
        const MediaPipeTracker &) = delete;

    TrackingFrame process(
        int width,
        int height,
        const std::vector<std::uint8_t> &rgb,
        std::int64_t captureUs,
        std::uint64_t sequence,
        bool swapHandedness);

private:
    void check(
        int code,
        char *error);

    QLibrary library_;
    std::vector<char> model_;

    mp::Landmarker handle_{};

    mp::Create create_{};
    mp::Detect detect_{};
    mp::CloseResult closeResult_{};
    mp::Close close_{};
    mp::ImageCreate imageCreate_{};
    mp::ImageFree imageFree_{};
    mp::ErrorFree errorFree_{};

    std::int64_t lastTimestampMs_{-1};
};

} // namespace vc
