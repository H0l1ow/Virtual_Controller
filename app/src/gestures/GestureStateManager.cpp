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
    result.hasTrackingStatus = true;

    for (std::size_t index = 0; index < kHandCount; ++index) {
        result.tracked[index] = predictions.hands[index].tracked;
        result.trackIds[index] = predictions.hands[index].tracked
            ? predictions.hands[index].trackId : 0;
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

GestureHandDebugState GestureStateManager::debugState(
    HandSide hand, std::int64_t timestampUs) const
{
    const auto &state = handStates_[handIndex(hand)];
    return {
        state.candidate,
        state.active,
        state.mismatchSinceUs != 0,
        state.blockedUntilReleaseObserved,
        timestampUs < state.cooldownUntilUs
    };
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

    const bool tracked = prediction.tracked && prediction.trackId != 0;
    const std::uint64_t identity = tracked ? prediction.trackId : 0;
    const GestureClass observed = prediction.ready
        ? prediction.gesture : GestureClass::None;

    // A physically lost hand releases immediately. Never grant a grace period
    // for loss of tracking or a changed identity: output must fail closed.
    if (!tracked || (state.trackId != 0 && state.trackId != identity)) {
        if (state.active != GestureClass::None) {
            event.gesture = state.active;
            event.phase = GestureEventPhase::Release;
            event.trackId = state.trackId;
        }
        state = {};
        state.trackId = identity;
        return event;
    }
    state.trackId = identity;

    // Deliberately ambiguous/unstable recognition is not a tracking dropout.
    // A held key/button from the preceding pose must not remain pressed while
    // the new pose is being disambiguated. Fail closed immediately.
    if (prediction.ready
        && (prediction.decision == GestureDecision::Ambiguous
            || prediction.decision == GestureDecision::PoseStabilizing)) {
        if (state.active != GestureClass::None) {
            event.gesture = state.active;
            event.phase = GestureEventPhase::Release;
            event.trackId = identity;
            state.cooldownUntilUs = timestampUs
                + static_cast<std::int64_t>(std::max(config.cooldownMs, 0)) * 1000;
        }
        state.active = GestureClass::None;
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        state.mismatchSinceUs = 0;
        state.blockedUntilReleaseObserved = false;
        state.blockedGesture = GestureClass::None;
        // Do not restart cooldown every unstable frame. It begins at the
        // actual RELEASE; otherwise a valid new pose could never stabilize
        // promptly when the user spends time forming it.
        return event;
    }

    if (state.active != GestureClass::None) {
        const bool sameGesture = prediction.ready
            && observed == state.active;
        const float exitThreshold =
            static_cast<float>(std::clamp(config.releaseThresholdPercent, 0, 100))
            / 100.0F;
        // Experimental poses have separate smoothed rules scores and can
        // use the same exit hysteresis without indexing the ONNX score vector.
        const auto activeIndex = gestureIndex(state.active);
        float fallbackScore = 0.0F;
        if (activeIndex < prediction.scores.size())
            fallbackScore = prediction.scores[activeIndex];
        else if (activeIndex < kStaticGestureCount)
            fallbackScore = prediction.experimentalScores[
                activeIndex - kGestureClassCount];
        const bool stillSupported = prediction.ready
            && observed == GestureClass::None
            && fallbackScore >= exitThreshold;

        if (sameGesture || stillSupported) {
            state.mismatchSinceUs = 0;
            event.gesture = state.active;
            event.phase = GestureEventPhase::Hold;
            event.confidence = sameGesture
                ? prediction.confidence
                : fallbackScore;
            event.trackId = identity;
            return event;
        }

        if (state.mismatchSinceUs == 0) {
            state.mismatchSinceUs = timestampUs;
        }
        const auto graceUs =
            static_cast<std::int64_t>(std::clamp(config.dropoutGraceMs, 0, 500))
            * 1000;
        if (timestampUs - state.mismatchSinceUs < graceUs) {
            event.gesture = state.active;
            event.phase = GestureEventPhase::Hold;
            event.confidence = fallbackScore;
            event.trackId = identity;
            return event;
        }

        const GestureClass released = state.active;
        state.active = GestureClass::None;
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        state.mismatchSinceUs = 0;
        state.cooldownUntilUs = timestampUs
            + static_cast<std::int64_t>(std::max(config.cooldownMs, 0)) * 1000;
        // An observation away from the released gesture already occurred.
        // Do not block a later valid re-press waiting for a second release.
        state.blockedUntilReleaseObserved =
            config.requireRelease && observed == released;
        state.blockedGesture = released;
        event.gesture = released;
        event.phase = GestureEventPhase::Release;
        event.trackId = identity;
        return event;
    }

    if (state.blockedUntilReleaseObserved) {
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        if (!prediction.ready || observed == state.blockedGesture) {
            return event;
        }
        state.blockedUntilReleaseObserved = false;
        state.blockedGesture = GestureClass::None;
        return event;
    }

    if (!prediction.ready || observed == GestureClass::None
        || timestampUs < state.cooldownUntilUs) {
        state.candidate = GestureClass::None;
        state.candidateSinceUs = 0;
        return event;
    }

    if (state.candidate != observed) {
        state.candidate = observed;
        state.candidateSinceUs = timestampUs;
        return event;
    }
    const auto debounceUs =
        static_cast<std::int64_t>(std::max(config.debounceMs, 0)) * 1000;
    if (timestampUs - state.candidateSinceUs < debounceUs) {
        return event;
    }
    state.active = observed;
    state.candidate = GestureClass::None;
    state.candidateSinceUs = 0;
    event.gesture = observed;
    event.phase = GestureEventPhase::Press;
    event.confidence = prediction.confidence;
    event.trackId = identity;
    return event;
}

} // namespace vc
