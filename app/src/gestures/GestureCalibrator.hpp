#pragma once

#include "gestures/GestureTypes.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace vc {

// A guided calibration samples existing gesture-class scores. Its output is
// an *assisted starting threshold*, NOT an accuracy/confidence guarantee.
// Both hands are sampled separately; a shared class threshold takes the
// weaker accepted (lower) recommendation so that neither is excluded.
class GestureCalibrator final {
public:
    enum class Status {
        Idle,
        Capturing,
        Ready,
        WeakSignal,
        Ambiguous,
        Cancelled
    };

    struct Result {
        bool valid{};
        int threshold{};
        int samples{};
        int dominantPercent{};
    };

    static constexpr int kRequiredSamples = 36;

    void start(HandSide side, GestureClass gesture);
    void cancel();
    void clearResults();
    void ingest(const GestureFrame &frame);

    bool capturing() const { return status_ == Status::Capturing; }
    Status status() const { return status_; }
    int sampleCount() const { return static_cast<int>(samples_.size()); }
    int progressPercent() const {
        return std::clamp(100 * sampleCount() / kRequiredSamples, 0, 100);
    }
    HandSide selectedSide() const { return side_; }
    GestureClass selectedGesture() const { return gesture_; }
    Result result(HandSide side, GestureClass gesture) const;
    int suggestedThreshold(GestureClass gesture) const;

private:
    struct Sample {
        float expected{};
        float competing{};
    };
    std::array<std::array<Result, kGestureClassCount>, kHandCount> results_{};
    std::vector<Sample> samples_;
    Status status_{Status::Idle};
    HandSide side_{HandSide::Right};
    GestureClass gesture_{GestureClass::Pinch};
    std::uint64_t activeTrackId_{};
    std::int64_t lastSampleUs_{};
    void finalize();
};

} // namespace vc
