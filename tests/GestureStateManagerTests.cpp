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
    right.tracked = ready;
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
    // Preserve the exact M4 transition tests without grace; M5 has separate
    // tests for real temporal gaps below.
    config.dropoutGraceMs = 0;

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

    // M5: one wrong/empty class while the physical hand is still visible is
    // not a RELEASE, and it must not block subsequent continuous HOLDs.
    manager.reset();
    config.dropoutGraceMs = 100;
    auto t = 2'000'000LL;
    auto pressFrame = frame(t, vc::GestureClass::Pinch);
    assert(rightEvent(manager.update(pressFrame, config)).phase == vc::GestureEventPhase::None);
    t += 120'000;
    assert(rightEvent(manager.update(frame(t, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::Press);
    t += 33'333;
    auto noise = frame(t, vc::GestureClass::None);
    assert(rightEvent(manager.update(noise, config)).phase == vc::GestureEventPhase::Hold);
    t += 33'333;
    assert(rightEvent(manager.update(frame(t, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::Hold);
    t += 33'333;
    assert(rightEvent(manager.update(frame(t, vc::GestureClass::Pinch), config)).phase
        == vc::GestureEventPhase::Hold);

    // The release threshold allows a low-confidence NONE while PINCH still
    // has a nontrivial model score, without starting a new grace interval.
    t += 33'333;
    auto weak = frame(t, vc::GestureClass::None);
    weak.hands[vc::handIndex(vc::HandSide::Right)]
        .scores[vc::gestureIndex(vc::GestureClass::Pinch)] = 0.65F;
    assert(rightEvent(manager.update(weak, config)).phase == vc::GestureEventPhase::Hold);

    // A persistent alternate gesture still releases after the grace period.
    t += 33'333;
    assert(rightEvent(manager.update(frame(t, vc::GestureClass::Fist), config)).phase
        == vc::GestureEventPhase::Hold);
    t += 120'000;
    assert(rightEvent(manager.update(frame(t, vc::GestureClass::Fist), config)).phase
        == vc::GestureEventPhase::Release);

    // Real loss or identity switch is never delayed by the grace period.
    manager.reset();
    manager.update(frame(3'000'000, vc::GestureClass::Pinch), config);
    manager.update(frame(3'120'000, vc::GestureClass::Pinch), config);
    assert(rightEvent(manager.update(frame(3'130'000, vc::GestureClass::None, false), config)).phase
        == vc::GestureEventPhase::Release);
    manager.update(frame(4'000'000, vc::GestureClass::Pinch), config);
    manager.update(frame(4'120'000, vc::GestureClass::Pinch), config);
    assert(rightEvent(manager.update(frame(4'130'000, vc::GestureClass::Pinch, true, 999), config)).phase
        == vc::GestureEventPhase::Release);

    return 0;
}
