#pragma once

#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vc {

inline constexpr std::size_t kGestureClassCount = 5;

enum class GestureClass : std::size_t {
    None = 0,
    Fist,
    OpenHand,
    Point,
    Pinch
};

inline constexpr std::array<const char *, kGestureClassCount>
    kGestureCanonicalNames{
        "NONE",
        "FIST",
        "OPEN_HAND",
        "POINT",
        "PINCH"
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
    GestureScores scores{}; // smoothed class scores; rules scores are not probabilities
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
