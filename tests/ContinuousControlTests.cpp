#include "control/ContinuousControlInterpreter.hpp"

#include <cassert>
#include <cmath>

namespace {

vc::HandTrackingState hand(
    std::uint64_t trackId,
    float x,
    float y,
    bool tracked = true)
{
    vc::HandTrackingState state;
    state.tracked = tracked;
    state.side = vc::HandSide::Right;
    state.trackId = tracked ? trackId : 0;
    state.palmPosition = {x, y, 0.0F};
    return state;
}

} // namespace

int main()
{
    vc::ContinuousControlInterpreter interpreter;
    vc::ContinuousControlConfig config;
    config.smoothing = false;
    config.sensitivity = 70;
    config.deadzone = 0;

    // First frame is a baseline and must never jump the cursor.
    auto first = interpreter.update(
        hand(1, 0.50F, 0.50F),
        1'000'000,
        config);

    assert(first.cursorTracked);
    assert(first.controller.neutral());

    auto moved = interpreter.update(
        hand(1, 0.51F, 0.49F),
        1'033'333,
        config);

    assert(moved.controller.mouseX > 0.0F);
    assert(moved.controller.mouseY < 0.0F);

    // DPI-like cursor speed multiplies the final motion without changing the
    // tracking baseline or the no-jump contract.
    interpreter.reset();
    config.cursorSpeedPercent = 100;
    interpreter.update(
        hand(10, 0.50F, 0.50F),
        1'100'000,
        config);
    const auto normalSpeed = interpreter.update(
        hand(10, 0.51F, 0.50F),
        1'133'333,
        config);

    interpreter.reset();
    config.cursorSpeedPercent = 200;
    interpreter.update(
        hand(11, 0.50F, 0.50F),
        1'200'000,
        config);
    const auto doubleSpeed = interpreter.update(
        hand(11, 0.51F, 0.50F),
        1'233'333,
        config);

    assert(doubleSpeed.controller.mouseX
        > normalSpeed.controller.mouseX * 1.9F);

    // Each axis can be inverted independently.
    interpreter.reset();
    config.cursorSpeedPercent = 100;
    config.invertX = true;
    config.invertY = true;
    interpreter.update(
        hand(12, 0.50F, 0.50F),
        1'300'000,
        config);
    const auto inverted = interpreter.update(
        hand(12, 0.51F, 0.49F),
        1'333'333,
        config);

    assert(inverted.controller.mouseX < 0.0F);
    assert(inverted.controller.mouseY > 0.0F);

    config.invertX = false;
    config.invertY = false;

    // Tiny motion inside the configured deadband is ignored.
    config.deadzone = 20;
    interpreter.reset();
    interpreter.update(
        hand(2, 0.50F, 0.50F),
        2'000'000,
        config);

    auto jitter = interpreter.update(
        hand(2, 0.501F, 0.50F),
        2'033'333,
        config);

    assert(jitter.controller.neutral());

    // Losing tracking resets history. Reacquisition is again a no-jump frame.
    auto lost = interpreter.update(
        hand(0, 0.0F, 0.0F, false),
        2'066'666,
        config);
    assert(lost.controller.neutral());

    auto reacquired = interpreter.update(
        hand(3, 0.80F, 0.20F),
        2'100'000,
        config);
    assert(reacquired.cursorTracked);
    assert(reacquired.controller.neutral());

    // Changing track identity while still detected must also prevent a jump.
    auto newIdentity = interpreter.update(
        hand(4, 0.20F, 0.80F),
        2'133'333,
        config);
    assert(newIdentity.controller.neutral());

    // Smoothing path should stay finite and preserve the no-jump contract.
    config.smoothing = true;
    config.deadzone = 0;
    interpreter.reset();
    interpreter.update(
        hand(5, 0.40F, 0.40F),
        3'000'000,
        config);
    auto smooth = interpreter.update(
        hand(5, 0.45F, 0.43F),
        3'033'333,
        config);
    assert(std::isfinite(smooth.controller.mouseX));
    assert(std::isfinite(smooth.controller.mouseY));

    return 0;
}
