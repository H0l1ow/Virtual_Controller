#include "gestures/GestureStateManager.hpp"

#include <cassert>

namespace {

vc::GestureFrame frame(
    std::int64_t timestampUs,
    vc::GestureClass gesture,
    bool ready = true,
    std::uint64_t trackId = 1)
{
    vc::GestureFrame result;
    result.timestampUs = timestampUs;
    result.sequence = static_cast<std::uint64_t>(timestampUs);
    auto &right = result.hands[vc::handIndex(vc::HandSide::Right)];
    right.ready = ready;
    right.gesture = gesture;
    right.confidence = ready ? 0.95F : 0.0F;
    right.trackId = ready ? trackId : 0;
    return result;
}

vc::GestureEvent rightEvent(
    const vc::GestureEventFrame &events)
{
    return events.hands[vc::handIndex(vc::HandSide::Right)];
}

} // namespace

int main()
{
    vc::GestureStateManager manager;
    vc::GestureStateConfig config;
    config.debounceMs = 100;
    config.cooldownMs = 200;
    config.requireRelease = true;

    assert(rightEvent(manager.update(frame(1'000'000, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::None);
    assert(rightEvent(manager.update(frame(1'050'000, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::None);

    auto event = rightEvent(manager.update(frame(1'100'000, vc::GestureClass::Pinch), config));
    assert(event.phase == vc::GestureEventPhase::Press);
    assert(event.gesture == vc::GestureClass::Pinch);

    event = rightEvent(manager.update(frame(1'130'000, vc::GestureClass::Pinch), config));
    assert(event.phase == vc::GestureEventPhase::Hold);

    event = rightEvent(manager.update(frame(1'160'000, vc::GestureClass::None), config));
    assert(event.phase == vc::GestureEventPhase::Release);
    assert(event.gesture == vc::GestureClass::Pinch);

    // Cooldown prevents immediate retrigger.
    assert(rightEvent(manager.update(frame(1'200'000, vc::GestureClass::OpenHand), config)).phase
        == vc::GestureEventPhase::None); // release observed away from PINCH
    assert(rightEvent(manager.update(frame(1'370'000, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::None); // candidate starts after cooldown
    event = rightEvent(manager.update(frame(1'470'000, vc::GestureClass::Pinch), config));
    assert(event.phase == vc::GestureEventPhase::Press);

    // Direct gesture change releases the active gesture. Require-release does
    // not demand NONE specifically: one complete observation away from the
    // released gesture is enough (e.g. PINCH -> OPEN_HAND/FIST).
    event = rightEvent(manager.update(frame(1'500'000, vc::GestureClass::Fist), config));
    assert(event.phase == vc::GestureEventPhase::Release);
    assert(event.gesture == vc::GestureClass::Pinch);

    assert(rightEvent(manager.update(frame(1'800'000, vc::GestureClass::Fist), config)).phase
        == vc::GestureEventPhase::None); // release observation
    assert(rightEvent(manager.update(frame(1'810'000, vc::GestureClass::Fist), config)).phase
        == vc::GestureEventPhase::None); // candidate starts
    event = rightEvent(manager.update(frame(1'910'000, vc::GestureClass::Fist), config));
    assert(event.phase == vc::GestureEventPhase::Press);

    // Tracking loss releases a held gesture instead of leaving output latched.
    event = rightEvent(manager.update(frame(1'950'000, vc::GestureClass::None, false), config));
    assert(event.phase == vc::GestureEventPhase::Release);
    assert(event.gesture == vc::GestureClass::Fist);

    return 0;
}
