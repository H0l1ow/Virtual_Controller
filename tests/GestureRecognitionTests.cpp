#include "gestures/GestureEngine.hpp"
#include "gestures/HandFeatures.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace {

using namespace vc;

Landmark point(float x, float y)
{
    return {x, y, 0.0F};
}

void setExtendedFinger(
    std::array<Landmark, kHandLandmarkCount> &p,
    int base,
    float x,
    float y,
    float step)
{
    p[base] = point(x, y);
    p[base + 1] = point(x, y - step);
    p[base + 2] = point(x, y - 2.0F * step);
    p[base + 3] = point(x, y - 3.0F * step);
}

void setCurledFinger(
    std::array<Landmark, kHandLandmarkCount> &p,
    int base,
    float x,
    float y)
{
    p[base] = point(x, y);
    p[base + 1] = point(x, y - 0.055F);
    p[base + 2] = point(x + 0.050F, y - 0.025F);
    p[base + 3] = point(x + 0.018F, y + 0.006F);
}

HandTrackingState makeHand(GestureClass gesture)
{
    HandTrackingState hand;
    hand.tracked = true;
    hand.side = HandSide::Right;
    hand.reportedSide = HandSide::Right;
    hand.handednessConfidence = 0.99F;
    hand.trackId = 7;
    hand.ageFrames = 20;
    hand.lastSeenUs = 1'000'000;

    auto &p = hand.landmarks;
    p[0] = point(0.50F, 0.82F);

    // Thumb is not part of curl calculation but is required by PINCH.
    p[1] = point(0.45F, 0.74F);
    p[2] = point(0.39F, 0.68F);
    p[3] = point(0.34F, 0.61F);
    p[4] = point(0.30F, 0.54F);

    setExtendedFinger(p, 5, 0.41F, 0.65F, 0.075F);
    setExtendedFinger(p, 9, 0.50F, 0.61F, 0.082F);
    setExtendedFinger(p, 13, 0.59F, 0.65F, 0.073F);
    setExtendedFinger(p, 17, 0.67F, 0.69F, 0.062F);

    if (gesture == GestureClass::Fist) {
        setCurledFinger(p, 5, 0.41F, 0.65F);
        setCurledFinger(p, 9, 0.50F, 0.61F);
        setCurledFinger(p, 13, 0.59F, 0.65F);
        setCurledFinger(p, 17, 0.67F, 0.69F);
    }
    else if (gesture == GestureClass::Point) {
        setCurledFinger(p, 9, 0.50F, 0.61F);
        setCurledFinger(p, 13, 0.59F, 0.65F);
        setCurledFinger(p, 17, 0.67F, 0.69F);
    }
    else if (gesture == GestureClass::Pinch) {
        // Put the thumb tip close to the extended index fingertip.
        p[4] = point(p[8].x - 0.008F, p[8].y + 0.006F);
    }

    return hand;
}

GestureClass bestGesture(const GestureScores &scores)
{
    const auto best = std::max_element(
        scores.begin() + 1,
        scores.end());

    return static_cast<GestureClass>(
        std::distance(scores.begin(), best));
}

void testRules()
{
    for (const auto gesture : {
             GestureClass::OpenHand,
             GestureClass::Fist,
             GestureClass::Point,
             GestureClass::Pinch}) {
        const auto hand = makeHand(gesture);
        const auto geometry =
            extractGestureGeometry(hand, 1000, 1000);

        assert(geometry.valid);

        const auto scores =
            ruleGestureScores(geometry);

        assert(bestGesture(scores) == gesture);
        assert(scores[gestureIndex(gesture)] > 0.55F);
    }
}

void testFeatureWindow()
{
    HandFeatureStream stream(16, 30);
    auto hand = makeHand(GestureClass::OpenHand);

    std::int64_t timestamp = 1'000'000;
    for (int frame = 0; frame < 20; ++frame) {
        hand.lastSeenUs = timestamp;
        stream.push(hand, 1000, 1000, timestamp);
        timestamp += 33'333;
    }

    assert(stream.ready());
    assert(stream.tensor().size()
           == 16U * kGestureFeatureCount);
}


void testLeftRightCanonicalization()
{
    const auto right = makeHand(GestureClass::Point);
    auto left = right;
    left.side = HandSide::Left;
    left.reportedSide = HandSide::Left;
    left.trackId = 8;

    for (auto &landmark : left.landmarks) {
        landmark.x = 1.0F - landmark.x;
    }

    HandFeatureExtractor rightExtractor;
    HandFeatureExtractor leftExtractor;

    const auto rightFeatures =
        rightExtractor.update(right, 1000, 1000, 1'000'000);
    const auto leftFeatures =
        leftExtractor.update(left, 1000, 1000, 1'000'000);

    for (std::size_t index = 0; index < 63U; ++index) {
        assert(std::abs(rightFeatures[index] - leftFeatures[index]) < 1.0e-5F);
    }

    for (std::size_t index = 126U; index < kGestureFeatureCount; ++index) {
        assert(std::abs(rightFeatures[index] - leftFeatures[index]) < 1.0e-5F);
    }
}

void testEngineThresholdAndReset()
{
    GestureEngine engine;
    TrackingFrame frame;
    frame.width = 1000;
    frame.height = 1000;
    frame.captureUs = 1'000'000;
    frame.sequence = 1;
    frame.hands[handIndex(HandSide::Right)] =
        makeHand(GestureClass::Fist);

    GestureFrame result;
    for (int index = 0; index < 6; ++index) {
        frame.captureUs += 33'333;
        frame.sequence++;
        frame.hands[handIndex(HandSide::Right)].lastSeenUs =
            frame.captureUs;
        result = engine.update(frame, 70);
    }

    const auto &right =
        result.hands[handIndex(HandSide::Right)];
    assert(right.ready);
    assert(right.gesture == GestureClass::Fist);
    assert(right.confidence >= 0.70F);

    frame.hands[handIndex(HandSide::Right)].tracked = false;
    frame.captureUs += 33'333;
    result = engine.update(frame, 70);

    const auto &lost =
        result.hands[handIndex(HandSide::Right)];
    assert(!lost.ready);
    assert(lost.gesture == GestureClass::None);
}

} // namespace

int main()
{
    testRules();
    testFeatureWindow();
    testLeftRightCanonicalization();
    testEngineThresholdAndReset();

    std::cout << "Gesture recognition tests passed\n";
    return 0;
}
