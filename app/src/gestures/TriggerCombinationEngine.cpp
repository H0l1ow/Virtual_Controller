#include "gestures/TriggerCombinationEngine.hpp"

#include "gestures/GestureRegistry.hpp"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace vc {

namespace {
constexpr std::int64_t milliseconds(int value)
{
    return static_cast<std::int64_t>(value) * 1000;
}
} // namespace

HandSide TriggerCombinationEngine::sideFor(std::size_t index)
{
    return index == handIndex(HandSide::Left) ? HandSide::Left : HandSide::Right;
}

void TriggerCombinationEngine::setMappings(const std::vector<MappingRule> &mappings)
{
    combinations_.clear();
    for (const auto &rule : mappings) {
        if (rule.enabled && rule.triggerKind == TriggerKind::TwoHandCombination
            && isRecognizedGesture(rule.gesture)
            && isRecognizedGesture(rule.rightGesture)
            && rule.synchronizationMs >= 0 && rule.synchronizationMs <= 1500
            && rule.overlapHoldMs >= 0 && rule.overlapHoldMs <= 1500) {
            combinations_.push_back(rule);
        }
    }
    reset();
}

void TriggerCombinationEngine::reset()
{
    hands_ = {};
    activePairId_.clear();
    previousTimestampUs_ = 0;
}

const MappingRule *TriggerCombinationEngine::matchingPair() const
{
    const auto &left = hands_[handIndex(HandSide::Left)];
    const auto &right = hands_[handIndex(HandSide::Right)];
    if (!left.active || !right.active || left.blocked || right.blocked
        || left.emitted || right.emitted || !left.trackId || !right.trackId) {
        return nullptr;
    }
    for (const auto &rule : combinations_) {
        if (left.gesture == rule.gesture && right.gesture == rule.rightGesture
            && std::llabs(left.sinceUs - right.sinceUs)
                   <= milliseconds(rule.synchronizationMs)) {
            return &rule;
        }
    }
    return nullptr;
}

bool TriggerCombinationEngine::waitForPossiblePair(
    std::size_t index, std::int64_t timestampUs) const
{
    const auto &hand = hands_[index];
    if (!hand.active || hand.blocked || hand.emitted)
        return false;
    const MappingRule *pair = matchingPair();
    if (pair) {
        const auto overlapSince = std::max(hands_[0].sinceUs, hands_[1].sinceUs);
        if (timestampUs - overlapSince < milliseconds(pair->overlapHoldMs))
            return true;
    }
    for (const auto &rule : combinations_) {
        const bool left = index == handIndex(HandSide::Left);
        if ((left ? rule.gesture : rule.rightGesture) == hand.gesture
            && timestampUs - hand.sinceUs < milliseconds(rule.synchronizationMs)) {
            return true;
        }
    }
    return false;
}

void TriggerCombinationEngine::emitSingle(
    ResolvedTriggerFrame &frame, std::size_t index,
    const HandState &hand, GestureEventPhase phase)
{
    frame.triggers.push_back({TriggerKind::Single, phase, sideFor(index),
                              hand.gesture, {}, hand.trackId});
}

void TriggerCombinationEngine::emitPair(
    ResolvedTriggerFrame &frame, GestureEventPhase phase) const
{
    frame.triggers.push_back({TriggerKind::TwoHandCombination, phase,
                              HandSide::Right, GestureClass::None,
                              activePairId_, 0});
}

ResolvedTriggerFrame TriggerCombinationEngine::update(const GestureEventFrame &events)
{
    ResolvedTriggerFrame result;
    result.timestampUs = events.timestampUs;
    result.hasTrackingStatus = events.hasTrackingStatus;
    result.tracked = events.tracked;
    result.trackIds = events.trackIds;

    if (events.timestampUs <= 0
        || (previousTimestampUs_ != 0 && events.timestampUs < previousTimestampUs_)) {
        // A clock discontinuity must not strand held outputs. Release all
        // previously dispatched triggers before resetting internal timers.
        if (!activePairId_.empty())
            emitPair(result, GestureEventPhase::Release);
        for (std::size_t i = 0; i < kHandCount; ++i) {
            if (hands_[i].active && hands_[i].emitted)
                emitSingle(result, i, hands_[i], GestureEventPhase::Release);
        }
        reset();
        return result;
    }
    previousTimestampUs_ = events.timestampUs;

    // A real hand loss/identity switch or a gesture RELEASE ends a pair on
    // this very frame, independent of the recognizer's dropout grace period.
    if (!activePairId_.empty()) {
        bool broken = false;
        for (std::size_t i = 0; i < kHandCount; ++i) {
            const auto &hand = hands_[i];
            const auto &event = events.hands[i];
            if (!hand.active || event.phase == GestureEventPhase::Release
                || (event.phase == GestureEventPhase::Press && event.gesture != hand.gesture)
                || (events.hasTrackingStatus && (!events.tracked[i]
                    || events.trackIds[i] != hand.trackId))) {
                broken = true;
            }
        }
        if (broken) {
            emitPair(result, GestureEventPhase::Release);
            activePairId_.clear();
            for (auto &hand : hands_) {
                // Never resurface one of the consumed gestures as a new press.
                hand.blocked = true;
                hand.emitted = false;
            }
        } else {
            emitPair(result, GestureEventPhase::Hold);
        }
    }

    for (std::size_t i = 0; i < kHandCount; ++i) {
        auto &hand = hands_[i];
        const auto &event = events.hands[i];
        const bool lost = events.hasTrackingStatus && (!events.tracked[i]
            || events.trackIds[i] == 0
            || (hand.trackId != 0 && events.trackIds[i] != hand.trackId));
        const bool released = lost || event.phase == GestureEventPhase::Release;

        if (released && hand.active) {
            if (hand.emitted) {
                emitSingle(result, i, hand, GestureEventPhase::Release);
            } else if (!hand.blocked && !lost && activePairId_.empty()) {
                // A short singleton was delayed for a possible combination.
                // Replay its PRESS as a one-shot; Hold output doesn't linger.
                emitSingle(result, i, hand, GestureEventPhase::Press);
                emitSingle(result, i, hand, GestureEventPhase::Release);
            }
            hand = {};
        }
        if (lost)
            continue;

        if (event.phase == GestureEventPhase::Press
            && isRecognizedGesture(event.gesture)) {
            if (hand.active && hand.emitted)
                emitSingle(result, i, hand, GestureEventPhase::Release);
            hand = {event.gesture, event.trackId, events.timestampUs, true, false, false};
        } else if (event.phase == GestureEventPhase::Hold
                   && hand.active && event.gesture == hand.gesture) {
            // Hand is still active. Dispatching occurs below after arbitration.
        }
    }

    // Once a combo is active, all involved singleton events stay suppressed.
    if (!activePairId_.empty())
        return result;

    if (const MappingRule *pair = matchingPair()) {
        const auto overlapSince = std::max(hands_[0].sinceUs, hands_[1].sinceUs);
        if (events.timestampUs - overlapSince >= milliseconds(pair->overlapHoldMs)) {
            activePairId_ = pair->id;
            for (auto &hand : hands_) {
                hand.blocked = true;
                hand.emitted = false;
            }
            emitPair(result, GestureEventPhase::Press);
            return result;
        }
    }

    for (std::size_t i = 0; i < kHandCount; ++i) {
        auto &hand = hands_[i];
        if (!hand.active || hand.blocked)
            continue;
        if (hand.emitted) {
            if (events.hands[i].phase == GestureEventPhase::Hold)
                emitSingle(result, i, hand, GestureEventPhase::Hold);
        } else if (!waitForPossiblePair(i, events.timestampUs)) {
            emitSingle(result, i, hand, GestureEventPhase::Press);
            hand.emitted = true;
        }
    }
    return result;
}

} // namespace vc
