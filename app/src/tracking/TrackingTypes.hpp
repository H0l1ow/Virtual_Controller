#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace vc {

inline constexpr std::size_t kHandCount = 2;
inline constexpr std::size_t kHandLandmarkCount = 21;

enum class HandSide : int {
    Left = 0,
    Right = 1
};

constexpr std::size_t handIndex(HandSide side)
{
    return side == HandSide::Left ? 0U : 1U;
}

constexpr HandSide oppositeHandSide(HandSide side)
{
    return side == HandSide::Left
        ? HandSide::Right
        : HandSide::Left;
}

struct Landmark {
    float x{};
    float y{};
    float z{};
};

// One raw MediaPipe detection. The side is the handedness reported by
// MediaPipe and is not yet treated as the stable application identity.
struct RawHandDetection {
    bool detected{};
    HandSide reportedSide{HandSide::Left};
    float handednessConfidence{};
    std::array<Landmark, kHandLandmarkCount> landmarks{};
};

struct RawTrackingFrame {
    int width{};
    int height{};

    std::int64_t captureUs{};
    std::int64_t trackStartUs{};
    std::int64_t trackEndUs{};

    std::uint64_t sequence{};

    std::array<RawHandDetection, kHandCount> detections{};
};

// Stable per-side state used by the rest of the application.
//
// handednessConfidence is intentionally named after what it actually means:
// the MediaPipe Left/Right classification score. It is not a generic tracking
// confidence value.
struct HandTrackingState {
    bool tracked{};

    // Stable logical identity used by mappings and future output layers.
    HandSide side{HandSide::Left};

    // Effective raw handedness for the currently assigned detection, after
    // the optional handedness swap setting has been applied.
    HandSide reportedSide{HandSide::Left};
    float handednessConfidence{};

    std::array<Landmark, kHandLandmarkCount> landmarks{};

    // Derived temporal state. Coordinates and velocity are in normalized
    // image space (velocity is normalized units / second).
    Landmark palmPosition{};
    Landmark velocity{};

    // trackId remains stable through a short occlusion/reacquisition window.
    // ageFrames counts successfully associated frames for this identity.
    std::uint64_t trackId{};
    std::uint32_t ageFrames{};
    std::int64_t lastSeenUs{};
};

struct TrackingFrame {
    int width{};
    int height{};

    std::int64_t captureUs{};
    std::int64_t trackStartUs{};
    std::int64_t trackEndUs{};

    std::uint64_t sequence{};

    std::array<HandTrackingState, kHandCount> hands{};
};

inline std::int64_t nowUs()
{
    using namespace std::chrono;

    return duration_cast<microseconds>(
               steady_clock::now().time_since_epoch())
        .count();
}

} // namespace vc
