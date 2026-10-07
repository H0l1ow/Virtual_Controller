#include "control/ContinuousControlInterpreter.hpp"

#include <algorithm>
#include <cmath>

namespace vc {

namespace {

float clampDelta(float value)
{
    return std::clamp(value, -200.0F, 200.0F);
}

} // namespace

float ContinuousControlInterpreter::cursorGain(
    int sensitivity)
{
    const float normalized =
        static_cast<float>(
            std::clamp(sensitivity, 0, 100))
        / 100.0F;

    // 70% -> 1140 px / normalized image unit. The range deliberately leaves
    // a usable low-sensitivity mode while still allowing fast large-screen
    // movement without requiring hand calibration in M2.
    return 300.0F + normalized * 1200.0F;
}

float ContinuousControlInterpreter::cursorDeadband(
    int deadzone)
{
    // UI percentage is converted to a tiny normalized per-frame movement
    // threshold. 12% corresponds to 0.0015, matching the reference cursor's
    // proven default mouse deadband.
    return static_cast<float>(
               std::clamp(deadzone, 0, 100))
        / 8000.0F;
}

ContinuousControlResult ContinuousControlInterpreter::update(
    const HandTrackingState &cursorHand,
    std::int64_t timestampUs,
    const ContinuousControlConfig &config)
{
    ContinuousControlResult result;

    if (!cursorHand.tracked
        || cursorHand.trackId == 0
        || timestampUs <= 0) {
        reset();
        return result;
    }

    const bool trackingRestarted =
        !initialized_
        || activeTrackId_ != cursorHand.trackId
        || timestampUs <= lastTimestampUs_
        || timestampUs - lastTimestampUs_ > 250000;

    if (trackingRestarted) {
        filterX_.reset();
        filterY_.reset();

        previousX_ = cursorHand.palmPosition.x;
        previousY_ = cursorHand.palmPosition.y;
        activeTrackId_ = cursorHand.trackId;
        lastTimestampUs_ = timestampUs;
        initialized_ = true;

        // Prime the filters while intentionally producing no cursor movement.
        if (config.smoothing) {
            previousX_ = filterX_.update(
                previousX_,
                timestampUs,
                config.cutoff,
                config.beta);
            previousY_ = filterY_.update(
                previousY_,
                timestampUs,
                config.cutoff,
                config.beta);
        }

        result.cursorTracked = true;
        result.filteredX = previousX_;
        result.filteredY = previousY_;
        return result;
    }

    float x = cursorHand.palmPosition.x;
    float y = cursorHand.palmPosition.y;

    if (config.smoothing) {
        x = filterX_.update(
            x,
            timestampUs,
            config.cutoff,
            config.beta);
        y = filterY_.update(
            y,
            timestampUs,
            config.cutoff,
            config.beta);
    }
    else {
        // Keep the filters clean while disabled. Re-enabling smoothing will
        // prime from the current position instead of stale filtered history.
        filterX_.reset();
        filterY_.reset();
    }

    float dx = x - previousX_;
    float dy = y - previousY_;

    const float movement =
        std::hypot(dx, dy);

    const float deadband =
        cursorDeadband(config.deadzone);

    if (movement <= deadband) {
        dx = 0.0F;
        dy = 0.0F;
        // Do not advance the movement baseline inside the deadband. Tiny
        // intentional motion can therefore accumulate until it becomes large
        // enough to pass the threshold instead of being discarded forever.
    }
    else {
        // Consume movement beyond the radial deadband without creating a
        // discontinuous jump at its boundary.
        const float scale =
            (movement - deadband) / movement;
        dx *= scale;
        dy *= scale;
        previousX_ = x;
        previousY_ = y;
    }

    lastTimestampUs_ = timestampUs;

    const float speedScale =
        static_cast<float>(
            std::clamp(config.cursorSpeedPercent, 50, 400))
        / 100.0F;

    const float gain =
        cursorGain(config.sensitivity) * speedScale;

    // The default horizontal mapping is reversed so hand motion feels natural
    // with the mirrored camera preview. The Invert X switch applies one more
    // reversal and therefore remains OFF in the default configuration.
    dx = -dx;

    if (config.invertX) {
        dx = -dx;
    }

    if (config.invertY) {
        dy = -dy;
    }

    result.controller.mouseX =
        clampDelta(dx * gain);
    result.controller.mouseY =
        clampDelta(dy * gain);
    result.cursorTracked = true;
    result.filteredX = x;
    result.filteredY = y;
    return result;
}

void ContinuousControlInterpreter::reset()
{
    filterX_.reset();
    filterY_.reset();
    previousX_ = 0.0F;
    previousY_ = 0.0F;
    activeTrackId_ = 0;
    lastTimestampUs_ = 0;
    initialized_ = false;
}

} // namespace vc
