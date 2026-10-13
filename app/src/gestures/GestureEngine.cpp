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
        state.trackId = 0;
        state.initialized = false;
        return prediction;
    }

    if (state.trackId != hand.trackId) {
        state.stream.reset();
        state.smoothed = {};
        state.trackId = hand.trackId;
        state.initialized = false;
    }

    const auto geometry = extractGestureGeometry(
        hand, frameWidth, frameHeight);
    if (!geometry.valid) {
        state.stream.reset();
        state.smoothed = {};
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
