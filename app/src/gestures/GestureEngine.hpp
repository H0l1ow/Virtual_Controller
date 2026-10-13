#pragma once

#include "gestures/GestureTypes.hpp"
#include "gestures/HandFeatures.hpp"
#include "gestures/GestureTuning.hpp"
#include "tracking/TrackingTypes.hpp"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace vc {

class GestureEngine final
{
public:
    using ModelInference =
        std::function<GestureScores(const std::vector<float> &)>;

    explicit GestureEngine(
        ModelInference modelInference = {},
        std::string backendName = "Rules");

    GestureFrame update(
        const TrackingFrame &tracking,
        int recognitionThresholdPercent);

    GestureFrame update(
        const TrackingFrame &tracking,
        const GestureThresholds &thresholds);

    void reset();
    void setPinchRange(HandSide side, float low, float high);

    bool usingModel() const
    {
        return static_cast<bool>(modelInference_);
    }

    const std::string &backendName() const
    {
        return backendName_;
    }

private:
    struct HandState {
        HandFeatureStream stream{};
        GestureScores smoothed{};
        std::uint64_t trackId{};
        bool initialized{};
    };

    GesturePrediction updateHand(
        HandState &state,
        const HandTrackingState &hand,
        int frameWidth,
        int frameHeight,
        std::int64_t timestampUs,
        const GestureThresholds &thresholds);

    static GestureScores smoothScores(
        const GestureScores &previous,
        const GestureScores &current,
        bool initialized);

    std::array<HandState, kHandCount> handStates_{};
    std::array<float, kHandCount> pinchLow_{0.17F, 0.17F};
    std::array<float, kHandCount> pinchHigh_{0.43F, 0.43F};
    ModelInference modelInference_;
    std::string backendName_;
};

} // namespace vc
