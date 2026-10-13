#pragma once

#include "tracking/TrackingTypes.hpp"
#include "gestures/HandFeatures.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace vc {

// Collects a tracked hand movement envelope or pinch distances. Results are
// advisory until the user explicitly applies them in Settings.
class HandRangeCalibrator final {
public:
    enum class Kind { Motion, PinchClosed, PinchOpen };
    enum class Status { Idle, Capturing, Ready, Insufficient, Cancelled };
    struct SideResult {
        bool motionValid{};
        float spanX{};
        float spanY{};
        bool closedValid{};
        float closedDistance{};
        bool openValid{};
        float openDistance{};
    };
    static constexpr int kSamples = 60;
    void start(HandSide side, Kind kind);
    void ingest(const TrackingFrame &frame);
    void cancel();
    void reset();
    bool capturing() const { return status_ == Status::Capturing; }
    Status status() const { return status_; }
    int progress() const { return static_cast<int>(100 * samples_ / kSamples); }
    SideResult result(HandSide side) const { return results_[handIndex(side)]; }
    HandSide selectedSide() const { return side_; }
    Kind selectedKind() const { return kind_; }
private:
    HandSide side_{HandSide::Right};
    Kind kind_{Kind::Motion};
    Status status_{Status::Idle};
    std::array<SideResult, kHandCount> results_{};
    int samples_{};
    std::uint64_t trackId_{};
    std::int64_t previousUs_{};
    float minX_{1.0F}, maxX_{}, minY_{1.0F}, maxY_{};
    std::vector<float> pinchValues_;
};

} // namespace vc
