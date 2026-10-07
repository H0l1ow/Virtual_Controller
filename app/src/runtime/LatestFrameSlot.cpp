#include "runtime/LatestFrameSlot.hpp"

#include <utility>

namespace vc {

LatestFrameSlot::PublishResult LatestFrameSlot::publish(
    VideoPacket packet)
{
    PublishResult result = PublishResult::Published;

    {
        std::lock_guard lock(mutex_);

        if (closed_) {
            return PublishResult::Closed;
        }

        if (packet_.has_value()) {
            result = PublishResult::Replaced;
        }

        packet_ = std::move(packet);
    }

    condition_.notify_one();
    return result;
}

std::optional<VideoPacket> LatestFrameSlot::take(
    std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mutex_);

    condition_.wait_for(
        lock,
        timeout,
        [this] {
            return closed_ || packet_.has_value();
        });

    if (!packet_) {
        return std::nullopt;
    }

    auto result = std::move(packet_);
    packet_.reset();
    return result;
}

void LatestFrameSlot::close()
{
    {
        std::lock_guard lock(mutex_);

        if (closed_) {
            return;
        }

        closed_ = true;
        packet_.reset();
    }

    condition_.notify_all();
}

bool LatestFrameSlot::isClosed() const
{
    std::lock_guard lock(mutex_);
    return closed_;
}

} // namespace vc
