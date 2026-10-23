#pragma once

#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vc {

inline constexpr std::size_t kGestureClassCount = 5;

// Preserve the five-class ONNX label order and output tensor size. The eight
// additional static poses are recognized by a separate landmark classifier.
// They are runtime gesture IDs, NOT extra ONNX output classes.
enum class GestureClass : std::size_t {
    None = 0,
    Fist,
    OpenHand,
    Point,
    Pinch,
    ThumbsUp,
    ThumbsDown,
    Victory,
    Rock,
    ThreeFingers,
    FourFingers,
    CallMe,
    Ok
};

inline constexpr std::size_t kStaticGestureCount = 13;
inline constexpr std::size_t kExperimentalStaticGestureCount =
    kStaticGestureCount - kGestureClassCount;
inline constexpr std::array<const char *, kStaticGestureCount>
    kGestureCanonicalNames{
        "NONE", "FIST", "OPEN_HAND", "POINT", "PINCH",
        "THUMBS_UP", "THUMBS_DOWN", "VICTORY", "ROCK",
        "THREE_FINGERS", "FOUR_FINGERS", "CALL_ME", "OK"
    };

constexpr std::size_t gestureIndex(GestureClass gesture)
{
    return static_cast<std::size_t>(gesture);
}

inline const char *gestureCanonicalName(GestureClass gesture)
{
    const auto index = gestureIndex(gesture);
    return index < kGestureCanonicalNames.size()
        ? kGestureCanonicalNames[index]
        : "NONE";
}

using GestureScores =
    std::array<float, kGestureClassCount>;

enum class GestureDecision : std::uint8_t {
    NoTracking,
    InvalidGeometry,
    ModelWarmup,
    BelowThreshold,
    NoneDominant,
    Ambiguous,
    PoseStabilizing,
    Recognized
};

inline const char *gestureDecisionName(GestureDecision decision)
{
    switch (decision) {
    case GestureDecision::NoTracking: return "NO_TRACKING";
    case GestureDecision::InvalidGeometry: return "INVALID_GEOMETRY";
    case GestureDecision::ModelWarmup: return "MODEL_WARMUP";
    case GestureDecision::BelowThreshold: return "BELOW_THRESHOLD";
    case GestureDecision::NoneDominant: return "NONE_DOMINANT";
    case GestureDecision::Ambiguous: return "AMBIGUOUS";
    case GestureDecision::PoseStabilizing: return "POSE_STABILIZING";
    case GestureDecision::Recognized: return "RECOGNIZED";
    }
    return "UNKNOWN";
}

struct GesturePrediction {
    GestureClass gesture{GestureClass::None};
    float confidence{};

    // ready means the recognizer had a valid tracked hand and produced a
    // meaningful prediction. NONE can therefore still be a ready prediction.
    bool ready{};
    // Tracking presence is independent of classification readiness, e.g.
    // while an ONNX temporal window fills or landmarks fail geometry checks.
    bool tracked{};
    bool geometryValid{};
    GestureDecision decision{GestureDecision::NoTracking};
    GestureScores scores{}; // five legacy/ONNX scores; not probabilities in Rules
    std::array<float, kExperimentalStaticGestureCount> experimentalScores{};
    // Comparable only within the experimental rules channel. These are
    // heuristic scores, not calibrated class probabilities/ONNX posteriors.
    GestureClass experimentalTop{GestureClass::None};
    GestureClass experimentalRunnerUp{GestureClass::None};
    float experimentalTopScore{};
    float experimentalRunnerUpScore{};
    float experimentalMargin{};
    int experimentalStableMs{};
    std::array<float, 4> fingerCurl{};
    float pinchDistance{};
    std::uint64_t trackId{};
};

struct GestureFrame {
    std::int64_t timestampUs{};
    std::uint64_t sequence{};
    std::array<GesturePrediction, kHandCount> hands{};
};

} // namespace vc
