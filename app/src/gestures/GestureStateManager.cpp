#include "gestures/GestureStateManager.hpp"

#include <algorithm>

namespace vc {

GestureEventFrame GestureStateManager::update(
    const GestureFrame &predictions,
    const GestureStateConfig &config)
{
    GestureEventFrame result;
    result.timestampUs = predictions.timestampUs;
    result.sequence = predictions.sequence;

    for (std::size_t index = 0; index < kHandCount; ++index) {
        result.hands[index] = updateHand(
            handStates_[index],
            predictions.hands[index],
            predictions.timestampUs,
            config);
    }

    return result;
}

void GestureStateManager::reset()
{
    handStates_ = {};
}

GestureEvent GestureStateManager::updateHand(
    HandState &state,
    const GesturePrediction &prediction,
    std::int64_t timestampUs,
    const GestureStateConfig &config)
{
    GestureEvent event;

    if (timestampUs <= 0) {
        state = {};
        return event;
    }

    const GestureClass observed = prediction.ready
        ? prediction.gesture
        : GestureClass::None;

    const std::uint64_t observedTrackId = prediction.ready
        ? prediction.trackId
        : 0;

    if (state.trackId != 0
        && observedTrackId != 0
        && observedTrackId != state.trackId) {
        // Identity changed. Release anything that was logically held, then
        // require a fresh debounce for the new hand identity.
        if (state.active != GestureClass::None) {
            event.gesture = state.active;
            event.phase = GestureEventPhase::Release;
            event.trackId = state.trackId;
        }

        state = {};
        state.trackId = observedTrackId;
        return event;
    }

    if (observedTrackId != 0) {
        state.trackId = observedTrackId;
    }

    if (state.active != GestureClass::None) {
        if (prediction.ready
            && observed == state.active
            && observedTrackId == state.trackId) {
            event.gesture = state.active;
            event.phase = GestureEventPhase::Hold;
            event.confidence = prediction.confidence;
            event.trackId = state.trackId;
            return event;
        }

        const GestureClass released = state.active;
        const std::uint64_t releasedTrack = state.trackId;

        state.active = GestureClass::None;
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        state.cooldownUntilUs = timestampUs
            + static_cast<std::int64_t>(
                std::max(config.cooldownMs, 0)) * 1000;
        state.blockedUntilReleaseObserved = config.requireRelease;
        state.blockedGesture = released;

        event.gesture = released;
        event.phase = GestureEventPhase::Release;
        event.trackId = releasedTrack;
        return event;
    }

    if (state.blockedUntilReleaseObserved) {
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;

        if (observed == state.blockedGesture) {
            return event;
        }

        // One full observation away from the released gesture is enough. It
        // does not have to be NONE; an OPEN_HAND release after PINCH is valid.
        state.blockedUntilReleaseObserved = false;
        state.blockedGesture = GestureClass::None;
        return event;
    }

    if (observed == GestureClass::None || !prediction.ready) {
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        if (!prediction.ready) {
            state.trackId = 0;
        }
        return event;
    }

    if (timestampUs < state.cooldownUntilUs) {
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        return event;
    }

    if (state.candidate != observed) {
        state.candidate = observed;
        state.candidateSinceUs = timestampUs;
        return event;
    }

    const std::int64_t debounceUs =
        static_cast<std::int64_t>(
            std::max(config.debounceMs, 0)) * 1000;

    if (timestampUs - state.candidateSinceUs < debounceUs) {
        return event;
    }

    state.active = observed;
    state.candidate = GestureClass::None;
    state.candidateSinceUs = 0;

    event.gesture = observed;
    event.phase = GestureEventPhase::Press;
    event.confidence = prediction.confidence;
    event.trackId = state.trackId;
    return event;
}

} // namespace vc
