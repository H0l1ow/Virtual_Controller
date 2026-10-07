#pragma once

#include <QVideoFrame>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>

namespace vc {

struct VideoPacket
{
    QVideoFrame frame;
    std::int64_t captureUs{};
    std::uint64_t sequence{};
};

// Single-slot mailbox. When inference is slower than the camera, a waiting
// frame is replaced by the newest one instead of building an old-frame queue.
class LatestFrameSlot final
{
public:
    enum class PublishResult {
        Published,
        Replaced,
        Closed
    };

    PublishResult publish(VideoPacket packet);

    std::optional<VideoPacket> take(
        std::chrono::milliseconds timeout);

    void close();
    bool isClosed() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::optional<VideoPacket> packet_;
    bool closed_{};
};

} // namespace vc
