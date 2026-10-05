#pragma once

#include <array>
#include <chrono>
#include <cstdint>

namespace vc {

enum class HandSide : int {
    Left = 0,
    Right = 1
};

struct Landmark {
    float x{};
    float y{};
    float z{};
};

struct HandTrackingState {
    bool tracked{};
    HandSide side{HandSide::Left};
    float confidence{};
    std::array<Landmark, 21> landmarks{};
};

struct TrackingFrame {
    int width{};
    int height{};

    std::int64_t captureUs{};
    std::int64_t trackStartUs{};
    std::int64_t trackEndUs{};

    std::uint64_t sequence{};

    std::array<HandTrackingState, 2> hands{};
};

inline std::int64_t nowUs()
{
    using namespace std::chrono;

    return duration_cast<microseconds>(
               steady_clock::now().time_since_epoch())
        .count();
}

} // namespace vc
