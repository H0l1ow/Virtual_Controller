#pragma once

#include <cstdint>

namespace vc {

// Limit rendering of SECONDARY camera previews. The primary QVideoSink,
// the tracking worker and the recorded dataset are never frame-throttled.
// Pure C++ to make the cadence testable without Qt or a real camera.
class PreviewFrameLimiter final
{
public:
    explicit PreviewFrameLimiter(int framesPerSecond = 12)
        : intervalUs_(1'000'000 / (framesPerSecond > 0 ? framesPerSecond : 12))
    {
    }

    bool shouldForward(std::int64_t timestampUs)
    {
        if (timestampUs <= 0)
            return false;
        if (previousUs_ == 0 || timestampUs < previousUs_
            || timestampUs - previousUs_ >= intervalUs_) {
            previousUs_ = timestampUs;
            return true;
        }
        return false;
    }

    void reset() { previousUs_ = 0; }

private:
    std::int64_t intervalUs_{};
    std::int64_t previousUs_{};
};

} // namespace vc
