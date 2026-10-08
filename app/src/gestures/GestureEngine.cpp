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

GestureFrame GestureEngine::update(
    const TrackingFrame &tracking,
    int recognitionThresholdPercent)
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
                recognitionThresholdPercent);
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
        state.pinchLatched = false;
    }
}

GesturePrediction GestureEngine::updateHand(
    HandState &state,
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight,
    std::int64_t timestampUs,
    int recognitionThresholdPercent)
{
    GesturePrediction prediction;
    prediction.trackId = hand.trackId;

    if (!hand.tracked
        || hand.trackId == 0
        || timestampUs <= 0) {
        state.stream.reset();
        state.smoothed = {};
        state.trackId = 0;
        state.initialized = false;
        state.pinchLatched = false;
        return prediction;
    }

    if (state.trackId != hand.trackId) {
        state.stream.reset();
        state.smoothed = {};
        state.trackId = hand.trackId;
        state.initialized = false;
        state.pinchLatched = false;
    }

    state.stream.push(
        hand,
        frameWidth,
        frameHeight,
        timestampUs);

    const auto geometry =
        extractGestureGeometry(
            hand,
            frameWidth,
            frameHeight);

    if (!geometry.valid) {
        state.pinchLatched = false;
    }
    else {
        const bool pinchShapeValid =
            geometry.curl[0] < 0.68F
            && geometry.thumbCurl < 0.70F;
        if (state.pinchLatched) {
            if (!pinchShapeValid
                || geometry.thumbIndexRuleDistance >= 0.46F) {
                state.pinchLatched = false;
            }
        }
        else if (pinchShapeValid
                 && geometry.thumbIndexRuleDistance <= 0.32F) {
            state.pinchLatched = true;
        }
    }

    // Static rules remain active even when a legacy five-class ONNX model is
    // present. This keeps the expanded static catalog usable without claiming
    // that the old model was trained for the new classes.
    GestureScores rawScores =
        ruleGestureScores(
            geometry,
            state.pinchLatched);

    if (modelInference_ && state.stream.ready()) {
        const GestureScores modelScores =
            modelInference_(state.stream.tensor());

        for (const auto gesture : kLegacyOnnxGestureClasses) {
            const auto index = gestureIndex(gesture);
            rawScores[index] =
                std::max(rawScores[index], modelScores[index]);
        }

        const float bestGesture =
            *std::max_element(
                rawScores.begin() + 1,
                rawScores.end());
        rawScores[gestureIndex(GestureClass::None)] =
            std::clamp(1.0F - bestGesture, 0.0F, 1.0F);
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

    const auto best =
        std::max_element(
            state.smoothed.begin() + 1,
            state.smoothed.end());

    const std::size_t bestIndex =
        static_cast<std::size_t>(
            std::distance(
                state.smoothed.begin(),
                best));

    const float threshold =
        static_cast<float>(
            std::clamp(
                recognitionThresholdPercent,
                0,
                100))
        / 100.0F;

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
    }
    else {
        prediction.gesture = GestureClass::None;
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
    constexpr float currentWeight = 0.70F;
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
