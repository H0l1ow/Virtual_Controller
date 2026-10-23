#include "gestures/GestureEngine.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace vc {

GestureEngine::GestureEngine(
    ModelInference modelInference,
    std::string backendName)
    : modelInference_(std::move(modelInference))
    , backendName_(std::move(backendName))
{
}

void GestureEngine::setPinchRange(HandSide side, float low, float high)
{
    if (std::isfinite(low) && std::isfinite(high) && low >= 0.02F && high > low + 0.03F) {
        pinchLow_[handIndex(side)] = low;
        pinchHigh_[handIndex(side)] = high;
    }
}

GestureFrame GestureEngine::update(
    const TrackingFrame &tracking,
    int recognitionThresholdPercent)
{
    GestureThresholds thresholds;
    thresholds.fallbackPercent = recognitionThresholdPercent;
    return update(tracking, thresholds);
}

GestureFrame GestureEngine::update(
    const TrackingFrame &tracking,
    const GestureThresholds &thresholds)
{
    GestureFrame result;
    result.timestampUs = tracking.captureUs;
    result.sequence = tracking.sequence;

    for (std::size_t index = 0;
         index < kHandCount;
         ++index) {
        result.hands[index] =
            updateHand(
                handStates_[index],
                tracking.hands[index],
                tracking.width,
                tracking.height,
                tracking.captureUs,
                thresholds);
    }

    return result;
}

void GestureEngine::reset()
{
    for (auto &state : handStates_) {
        state.stream.reset();
        state.smoothed = {};
        state.experimentalSmoothed = {};
        state.experimentalCandidate = GestureClass::None;
        state.experimentalCandidateSinceUs = 0;
        state.previousTimestampUs = 0;
        state.trackId = 0;
        state.initialized = false;
    }
}

GesturePrediction GestureEngine::updateHand(
    HandState &state,
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight,
    std::int64_t timestampUs,
    const GestureThresholds &thresholds)
{
    GesturePrediction prediction;
    prediction.trackId = hand.trackId;
    prediction.tracked = hand.tracked && hand.trackId != 0;

    if (!hand.tracked
        || hand.trackId == 0
        || timestampUs <= 0) {
        state.stream.reset();
        state.smoothed = {};
        state.experimentalSmoothed = {};
        state.experimentalCandidate = GestureClass::None;
        state.experimentalCandidateSinceUs = 0;
        state.previousTimestampUs = 0;
        state.trackId = 0;
        state.initialized = false;
        return prediction;
    }

    // A stalled capture must not preserve a stable pose across missing frames.
    const bool staleStream = state.previousTimestampUs > 0
        && (timestampUs <= state.previousTimestampUs
            || timestampUs - state.previousTimestampUs > kGestureMaxGapUs);
    if (state.trackId != hand.trackId || staleStream) {
        state.stream.reset();
        state.smoothed = {};
        state.experimentalSmoothed = {};
        state.experimentalCandidate = GestureClass::None;
        state.experimentalCandidateSinceUs = 0;
        state.trackId = hand.trackId;
        state.initialized = false;
    }
    state.previousTimestampUs = timestampUs;

    const auto geometry = extractGestureGeometry(
        hand, frameWidth, frameHeight);
    if (!geometry.valid) {
        state.stream.reset();
        state.smoothed = {};
        state.experimentalSmoothed = {};
        state.experimentalCandidate = GestureClass::None;
        state.experimentalCandidateSinceUs = 0;
        state.initialized = false;
        prediction.decision = GestureDecision::InvalidGeometry;
        return prediction;
    }
    prediction.geometryValid = true;
    prediction.fingerCurl = geometry.curl;
    prediction.pinchDistance = geometry.pinch[0];

    state.stream.push(hand, frameWidth, frameHeight, timestampUs);
    GestureScores rawScores{};
    if (modelInference_) {
        if (!state.stream.ready()) {
            // Never dispatch rule events while the ONNX sequence warms up.
            prediction.decision = GestureDecision::ModelWarmup;
            return prediction;
        }
        rawScores = modelInference_(state.stream.tensor());
    }
    else {
        const auto side = handIndex(hand.side);
        rawScores = ruleGestureScores(geometry, pinchLow_[side], pinchHigh_[side]);
    }

    for (auto &score : rawScores) {
        if (!std::isfinite(score)) {
            score = 0.0F;
        }
        score = std::clamp(score, 0.0F, 1.0F);
    }

    state.smoothed =
        smoothScores(
            state.smoothed,
            rawScores,
            state.initialized);
    state.initialized = true;
    prediction.scores = state.smoothed;

    const auto best =
        std::max_element(
            state.smoothed.begin() + 1,
            state.smoothed.end());

    const std::size_t bestIndex =
        static_cast<std::size_t>(
            std::distance(
                state.smoothed.begin(),
                best));

    const auto bestClass = static_cast<GestureClass>(bestIndex);
    const float threshold =
        static_cast<float>(thresholds.forClass(bestClass)) / 100.0F;

    const float noneScore =
        state.smoothed[
            gestureIndex(GestureClass::None)];

    prediction.ready = true;

    // This channel recognizes the eight additional poses independently of
    // the immutable five-class ONNX contract. Scores within this channel
    // are comparable; ONNX probabilities and rules scores are NOT directly
    // comparable. Never choose a class by comparing those two scales.
    const auto experimental = experimentalStaticScores(geometry);
    const float rawExtraPeak = *std::max_element(experimental.begin(), experimental.end());
    for (std::size_t i = 0; i < experimental.size(); ++i) {
        state.experimentalSmoothed[i] =
            state.experimentalSmoothed[i] * 0.60F + experimental[i] * 0.40F;
    }
    prediction.experimentalScores = state.experimentalSmoothed;

    std::size_t first = 0;
    std::size_t second = 1;
    if (state.experimentalSmoothed[second] > state.experimentalSmoothed[first])
        std::swap(first, second);
    for (std::size_t i = 2; i < state.experimentalSmoothed.size(); ++i) {
        if (state.experimentalSmoothed[i] > state.experimentalSmoothed[first]) {
            second = first;
            first = i;
        } else if (state.experimentalSmoothed[i] > state.experimentalSmoothed[second]) {
            second = i;
        }
    }
    prediction.experimentalTop = experimentalStaticClass(first);
    prediction.experimentalRunnerUp = experimentalStaticClass(second);
    prediction.experimentalTopScore = state.experimentalSmoothed[first];
    prediction.experimentalRunnerUpScore = state.experimentalSmoothed[second];
    prediction.experimentalMargin = prediction.experimentalTopScore
        - prediction.experimentalRunnerUpScore;

    // An unclear new pose fails closed instead of activating the old FIST,
    // PINCH or OPEN_HAND mapping while the hand is still forming the pose.
    constexpr float kPotentialPose = 0.64F;
    constexpr float kMinimumMargin = 0.14F;
    constexpr std::int64_t kMinimumStableUs = 120'000;
    const float extraThreshold = std::max(
        0.82F, static_cast<float>(thresholds.fallbackPercent) / 100.0F);
    // React to raw evidence immediately; waiting for EMA to cross the guard
    // would briefly expose an old keyboard/mouse mapping during a new pose.
    if (prediction.experimentalTopScore >= kPotentialPose
        || rawExtraPeak >= 0.80F) {
        prediction.confidence = prediction.experimentalTopScore;
        if (prediction.experimentalTopScore < kPotentialPose) {
            state.experimentalCandidate = GestureClass::None;
            state.experimentalCandidateSinceUs = 0;
            prediction.decision = GestureDecision::PoseStabilizing;
            return prediction;
        }
        if (prediction.experimentalMargin < kMinimumMargin) {
            state.experimentalCandidate = GestureClass::None;
            state.experimentalCandidateSinceUs = 0;
            prediction.decision = GestureDecision::Ambiguous;
            return prediction;
        }
        if (prediction.experimentalTopScore < extraThreshold) {
            state.experimentalCandidate = GestureClass::None;
            state.experimentalCandidateSinceUs = 0;
            prediction.decision = GestureDecision::BelowThreshold;
            return prediction;
        }
        if (state.experimentalCandidate != prediction.experimentalTop) {
            state.experimentalCandidate = prediction.experimentalTop;
            state.experimentalCandidateSinceUs = timestampUs;
        }
        prediction.experimentalStableMs = static_cast<int>(
            std::max<std::int64_t>(0, timestampUs - state.experimentalCandidateSinceUs)
            / 1000);
        if (timestampUs - state.experimentalCandidateSinceUs < kMinimumStableUs) {
            prediction.decision = GestureDecision::PoseStabilizing;
            return prediction;
        }
        prediction.gesture = prediction.experimentalTop;
        prediction.decision = GestureDecision::Recognized;
        return prediction;
    }
    state.experimentalCandidate = GestureClass::None;
    state.experimentalCandidateSinceUs = 0;

    if (best != state.smoothed.end()
        && *best >= threshold
        && *best >= noneScore) {
        prediction.gesture =
            static_cast<GestureClass>(bestIndex);
        prediction.confidence = *best;
        prediction.decision = GestureDecision::Recognized;
    }
    else {
        prediction.gesture = GestureClass::None;
        prediction.decision = best != state.smoothed.end() && *best >= threshold
            ? GestureDecision::NoneDominant
            : GestureDecision::BelowThreshold;
        prediction.confidence =
            std::max(noneScore, 1.0F - (best != state.smoothed.end() ? *best : 0.0F));
    }

    return prediction;
}

GestureScores GestureEngine::smoothScores(
    const GestureScores &previous,
    const GestureScores &current,
    bool initialized)
{
    if (!initialized) {
        return current;
    }

    GestureScores result{};
    constexpr float currentWeight = 0.45F;
    constexpr float previousWeight = 1.0F - currentWeight;

    for (std::size_t index = 0;
         index < result.size();
         ++index) {
        result[index] =
            previous[index] * previousWeight
            + current[index] * currentWeight;
    }

    return result;
}

} // namespace vc
