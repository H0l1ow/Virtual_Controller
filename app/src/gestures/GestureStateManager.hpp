#pragma once

#include "gestures/GestureTypes.hpp"

#include <array>
#include <cstdint>

namespace vc {

enum class GestureEventPhase : std::uint8_t {
    None = 0,
    Press,
    Hold,
    Release
};

inline const char *gestureEventPhaseName(GestureEventPhase phase)
{
    switch (phase) {
    case GestureEventPhase::Press:
        return "PRESS";
    case GestureEventPhase::Hold:
        return "HOLD";
    case GestureEventPhase::Release:
        return "RELEASE";
    case GestureEventPhase::None:
    default:
        return "IDLE";
    }
}

struct GestureEvent {
    GestureClass gesture{GestureClass::None};
    GestureEventPhase phase{GestureEventPhase::None};
    float confidence{};
    std::uint64_t trackId{};
};

struct GestureEventFrame {
    std::int64_t timestampUs{};
    std::uint64_t sequence{};
    std::array<GestureEvent, kHandCount> hands{};
};

struct GestureStateConfig {
    int debounceMs{120};
    int cooldownMs{250};
    bool requireRelease{true};
};

class GestureStateManager final
{
public:
    GestureEventFrame update(
        const GestureFrame &predictions,
        const GestureStateConfig &config);

    void reset();

private:
    struct HandState {
        GestureClass candidate{GestureClass::None};
        GestureClass active{GestureClass::None};
        std::int64_t candidateSinceUs{};
        std::int64_t cooldownUntilUs{};
        std::uint64_t trackId{};
        GestureClass blockedGesture{GestureClass::None};
        bool blockedUntilReleaseObserved{};
    };

    static GestureEvent updateHand(
        HandState &state,
        const GesturePrediction &prediction,
        std::int64_t timestampUs,
        const GestureStateConfig &config);

    std::array<HandState, kHandCount> handStates_{};
};

} // namespace vc
