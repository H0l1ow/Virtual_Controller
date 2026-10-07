#pragma once

#include "control/ControllerState.hpp"
#include "control/OneEuroFilter.hpp"
#include "tracking/TrackingTypes.hpp"

#include <cstdint>

namespace vc {

struct ContinuousControlConfig {
    int sensitivity{70};
    int cursorSpeedPercent{250};
    int deadzone{12};
    bool smoothing{true};
    bool invertX{false};
    bool invertY{false};

    // One Euro defaults intentionally match the proven cursor behavior from
    // the reference project rather than gesture-model settings.
    float cutoff{1.5F};
    float beta{0.35F};
};

struct ContinuousControlResult {
    ControllerState controller{};
    bool cursorTracked{};
    float filteredX{};
    float filteredY{};
};

class ContinuousControlInterpreter final
{
public:
    ContinuousControlResult update(
        const HandTrackingState &cursorHand,
        std::int64_t timestampUs,
        const ContinuousControlConfig &config);

    void reset();

private:
    static float cursorGain(int sensitivity);
    static float cursorDeadband(int deadzone);

    OneEuroFilter filterX_;
    OneEuroFilter filterY_;

    float previousX_{};
    float previousY_{};
    std::uint64_t activeTrackId_{};
    std::int64_t lastTimestampUs_{};
    bool initialized_{};
};

} // namespace vc
