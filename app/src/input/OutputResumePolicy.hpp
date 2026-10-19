#pragma once

#include <cstdint>

namespace vc {

// Fresh, consecutive *tracked* frames are required after a transient pause.
// Never replay an old controller state after a camera/GUI stall. The output
// service owns the user's persistent request separately from this policy.
class OutputResumePolicy final
{
public:
    void reset() noexcept
    {
        count_ = 0;
        firstCaptureUs_ = 0;
        lastCaptureUs_ = 0;
        lastSequence_ = 0;
    }

    [[nodiscard]] bool observe(
        std::int64_t captureUs,
        std::uint64_t sequence,
        std::int64_t nowUs,
        bool handTracked) noexcept
    {
        constexpr std::int64_t kFreshnessUs = 250'000;
        constexpr std::int64_t kMaxGapUs = 180'000;
        constexpr std::int64_t kStableWindowUs = 120'000;
        constexpr unsigned kMinFrames = 4;

        if (!handTracked || captureUs <= 0 || captureUs > nowUs
            || nowUs - captureUs > kFreshnessUs) {
            reset();
            return false;
        }

        if (count_ > 0
            && (captureUs <= lastCaptureUs_
                || sequence <= lastSequence_
                || captureUs - lastCaptureUs_ > kMaxGapUs)) {
            reset();
        }

        if (count_ == 0)
            firstCaptureUs_ = captureUs;
        ++count_;
        lastCaptureUs_ = captureUs;
        lastSequence_ = sequence;
        return count_ >= kMinFrames
            && captureUs - firstCaptureUs_ >= kStableWindowUs;
    }

private:
    unsigned count_{};
    std::int64_t firstCaptureUs_{};
    std::int64_t lastCaptureUs_{};
    std::uint64_t lastSequence_{};
};

} // namespace vc
