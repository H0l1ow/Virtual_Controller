#pragma once

#include "gestures/GestureTypes.hpp"
#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace vc {

inline constexpr std::size_t kGestureFeatureCount = 134;
inline constexpr int kGestureSampleRate = 30;
inline constexpr std::int64_t kGestureMaxGapUs = 150000;
inline constexpr const char *kGestureFeatureSchema = "vc.hand134.v2";

struct GestureGeometry {
    std::array<Landmark, kHandLandmarkCount> local{};
    Landmark palm{};
    float scale{};

    // Index, middle, ring and little finger curl in [0,1].
    std::array<float, 4> curl{};

    // Thumb-to-index and thumb-to-middle distances in normalized local space.
    std::array<float, 2> pinch{};

    bool valid{};
};

GestureGeometry extractGestureGeometry(
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight);

using GestureFeatures =
    std::array<float, kGestureFeatureCount>;

class HandFeatureExtractor final
{
public:
    GestureFeatures update(
        const HandTrackingState &hand,
        int frameWidth,
        int frameHeight,
        std::int64_t sampleUs);

    void reset();

private:
    std::array<Landmark, kHandLandmarkCount> previous_{};
    std::int64_t lastUs_{};
    bool initialized_{};
};

// Causal zero-order hold sampled on a fixed grid. Older samples never use a
// future camera observation. This is the exact contract expected by the M3
// causal TCN and mirrors the Python feature pipeline.
class HandFeatureStream final
{
public:
    explicit HandFeatureStream(
        int windowSize = 16,
        int sampleRate = kGestureSampleRate);

    bool push(
        const HandTrackingState &hand,
        int frameWidth,
        int frameHeight,
        std::int64_t timestampUs);

    void reset();

    bool ready() const
    {
        return static_cast<int>(window_.size()) == windowSize_;
    }

    int windowSize() const
    {
        return windowSize_;
    }

    std::vector<float> tensor() const;

private:
    struct Observation {
        HandTrackingState hand{};
        int width{};
        int height{};
        std::int64_t timestampUs{};
    };

    HandFeatureExtractor extractor_;
    std::optional<Observation> previous_;
    std::deque<GestureFeatures> window_;

    std::int64_t nextSampleUs_{};
    std::int64_t periodUs_{};
    int windowSize_{};
};

GestureScores ruleGestureScores(
    const GestureGeometry &geometry);

} // namespace vc
