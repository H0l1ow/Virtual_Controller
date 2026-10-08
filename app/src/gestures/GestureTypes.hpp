#pragma once

#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vc {

inline constexpr std::size_t kGestureClassCount = 22;
inline constexpr std::size_t kMappingGestureCount = 21;
inline constexpr std::size_t kLegacyOnnxGestureClassCount = 5;

enum class GestureClass : std::size_t {
    None = 0,
    Fist,
    OpenHand,
    Point,
    Pinch,
    ThumbUp,
    ThumbDown,
    Victory,
    Ok,
    ILoveYou,
    Rock,
    CallMe,
    ThreeFingers,
    FourFingers,
    SwipeLeft,
    SwipeRight,
    SwipeUp,
    SwipeDown,
    CircleCw,
    CircleCcw,
    Push,
    Pull
};

inline constexpr std::array<const char *, kGestureClassCount>
    kGestureCanonicalNames{
        "NONE",
        "FIST",
        "OPEN_HAND",
        "POINT",
        "PINCH",
        "THUMB_UP",
        "THUMB_DOWN",
        "VICTORY",
        "OK",
        "I_LOVE_YOU",
        "ROCK",
        "CALL_ME",
        "THREE_FINGERS",
        "FOUR_FINGERS",
        "SWIPE_LEFT",
        "SWIPE_RIGHT",
        "SWIPE_UP",
        "SWIPE_DOWN",
        "CIRCLE_CW",
        "CIRCLE_CCW",
        "PUSH",
        "PULL"
    };

// The optional ONNX path intentionally keeps the original five-class model
// contract until a real expanded dataset exists. Runtime rule recognition can
// expose more gestures without pretending an untrained 22-class model exists.
inline constexpr std::array<GestureClass, kLegacyOnnxGestureClassCount>
    kLegacyOnnxGestureClasses{
        GestureClass::None,
        GestureClass::Fist,
        GestureClass::OpenHand,
        GestureClass::Point,
        GestureClass::Pinch
    };

inline constexpr std::array<const char *, kLegacyOnnxGestureClassCount>
    kLegacyOnnxGestureCanonicalNames{
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
