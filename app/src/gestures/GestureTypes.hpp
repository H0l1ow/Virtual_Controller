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

struct GesturePrediction {
    GestureClass gesture{GestureClass::None};
    float confidence{};

    // ready means the recognizer had a valid tracked hand and produced a
    // meaningful prediction. NONE can therefore still be a ready prediction.
    bool ready{};

    std::uint64_t trackId{};
};

struct GestureFrame {
    std::int64_t timestampUs{};
    std::uint64_t sequence{};
    std::array<GesturePrediction, kHandCount> hands{};
};

} // namespace vc
