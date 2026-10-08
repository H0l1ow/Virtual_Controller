#include "gestures/GestureEngine.hpp"
#include "gestures/GestureRegistry.hpp"
#include "gestures/HandFeatures.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <string>

namespace {

using namespace vc;

Landmark point(float x, float y, float z = 0.0F)
{
    return {x, y, z};
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

void setPartiallyBentIndex(
    std::array<Landmark, kHandLandmarkCount> &p)
{
    p[5] = point(0.41F, 0.65F);
    p[6] = point(0.41F, 0.59F);
    p[7] = point(0.43F, 0.52F);
    p[8] = point(0.43F, 0.45F);
}

void setThumbExtendedUp(std::array<Landmark, kHandLandmarkCount> &p)
{
    p[1] = point(0.45F, 0.74F);
    p[2] = point(0.39F, 0.68F);
    p[3] = point(0.34F, 0.61F);
    p[4] = point(0.30F, 0.54F);
}

void setThumbExtendedDown(std::array<Landmark, kHandLandmarkCount> &p)
{
    p[1] = point(0.45F, 0.74F);
    p[2] = point(0.42F, 0.79F);
    p[3] = point(0.40F, 0.85F);
    p[4] = point(0.39F, 0.92F);
}

void setThumbFolded(std::array<Landmark, kHandLandmarkCount> &p)
{
    p[1] = point(0.45F, 0.74F);
    p[2] = point(0.40F, 0.70F);
    p[3] = point(0.42F, 0.65F);
    p[4] = point(0.46F, 0.72F);
}

HandTrackingState makeBaseHand()
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
    setThumbExtendedUp(p);
    setExtendedFinger(p, 5, 0.41F, 0.65F, 0.075F);
    setExtendedFinger(p, 9, 0.50F, 0.61F, 0.082F);
    setExtendedFinger(p, 13, 0.59F, 0.65F, 0.073F);
    setExtendedFinger(p, 17, 0.67F, 0.69F, 0.062F);
    return hand;
}

HandTrackingState makeHand(GestureClass gesture)
{
    auto hand = makeBaseHand();
    auto &p = hand.landmarks;

    const auto curlIndex = [&] { setCurledFinger(p, 5, 0.41F, 0.65F); };
    const auto curlMiddle = [&] { setCurledFinger(p, 9, 0.50F, 0.61F); };
    const auto curlRing = [&] { setCurledFinger(p, 13, 0.59F, 0.65F); };
    const auto curlLittle = [&] { setCurledFinger(p, 17, 0.67F, 0.69F); };

    switch (gesture) {
    case GestureClass::Fist:
        curlIndex(); curlMiddle(); curlRing(); curlLittle();
        setThumbFolded(p);
        break;
    case GestureClass::Point:
        curlMiddle(); curlRing(); curlLittle();
        setThumbFolded(p);
        break;
    case GestureClass::Pinch:
        setPartiallyBentIndex(p);
        curlMiddle(); curlRing(); curlLittle();
        p[4] = point(p[8].x - 0.006F, p[8].y + 0.004F);
        break;
    case GestureClass::ThumbUp:
        curlIndex(); curlMiddle(); curlRing(); curlLittle();
        setThumbExtendedUp(p);
        break;
    case GestureClass::ThumbDown:
        curlIndex(); curlMiddle(); curlRing(); curlLittle();
        setThumbExtendedDown(p);
        break;
    case GestureClass::Victory:
        curlRing(); curlLittle();
        setThumbFolded(p);
        break;
    case GestureClass::Ok:
        p[4] = point(p[8].x - 0.006F, p[8].y + 0.004F);
        break;
    case GestureClass::ILoveYou:
        curlMiddle(); curlRing();
        setThumbExtendedUp(p);
        break;
    case GestureClass::Rock:
        curlMiddle(); curlRing();
        setThumbFolded(p);
        break;
    case GestureClass::CallMe:
        curlIndex(); curlMiddle(); curlRing();
        setThumbExtendedUp(p);
        break;
    case GestureClass::ThreeFingers:
        curlLittle();
        setThumbFolded(p);
        break;
    case GestureClass::FourFingers:
        setThumbFolded(p);
        break;
    case GestureClass::OpenHand:
    default:
        break;
    }

    return hand;
}

GestureClass bestGesture(const GestureScores &scores)
{
    const auto best = std::max_element(scores.begin() + 1, scores.end());
    return static_cast<GestureClass>(std::distance(scores.begin(), best));
}

void assertRecognized(GestureClass expected)
{
    const auto hand = makeHand(expected);
    const auto geometry = extractGestureGeometry(hand, 1000, 1000);
    assert(geometry.valid);
    const auto scores = ruleGestureScores(geometry, expected == GestureClass::Pinch);
    if (bestGesture(scores) != expected) {
        std::cerr << "Expected " << gestureCanonicalName(expected)
                  << " but got " << gestureCanonicalName(bestGesture(scores)) << "\n";
        for (std::size_t i = 1; i < scores.size(); ++i) {
            if (scores[i] > 0.05F) {
                std::cerr << "  " << kGestureCanonicalNames[i] << ": " << scores[i] << "\n";
            }
        }
    }
    assert(bestGesture(scores) == expected);
    assert(scores[gestureIndex(expected)] > 0.80F);
}

void testStaticRuleCatalog()
{
    for (const auto gesture : {
             GestureClass::OpenHand,
             GestureClass::Fist,
             GestureClass::Point,
             GestureClass::Pinch,
             GestureClass::ThumbUp,
             GestureClass::ThumbDown,
             GestureClass::Victory,
             GestureClass::Ok,
             GestureClass::ILoveYou,
             GestureClass::Rock,
             GestureClass::CallMe,
             GestureClass::ThreeFingers,
             GestureClass::FourFingers}) {
        assertRecognized(gesture);
        assert(gestureRuntimeAvailable(gesture));
    }
}

void testRegistryHasTwentyOneMappingGestures()
{
    assert(kGestureDefinitions.size() == 21U);
    std::set<std::string> canonical;
    for (const auto &definition : kGestureDefinitions) {
        const auto inserted = canonical.insert(gestureCanonicalName(definition.gesture));
        assert(inserted.second);
        assert(definition.gesture != GestureClass::None);
    }

    int staticCount = 0;
    int temporalCount = 0;
    for (const auto &definition : kGestureDefinitions) {
        if (gestureRuntimeAvailable(definition.gesture)) ++staticCount;
        if (gestureTemporalPlanned(definition.gesture)) ++temporalCount;
    }
    assert(staticCount == 13);
    assert(temporalCount == 8);
}

void testPinchBeatsFistWithCurledOtherFingers()
{
    auto hand = makeHand(GestureClass::Pinch);
    const auto geometry = extractGestureGeometry(hand, 1000, 1000);
    assert(geometry.valid);
    assert(geometry.curl[1] > 0.50F);
    assert(geometry.curl[2] > 0.50F);
    assert(geometry.curl[3] > 0.50F);

    const auto scores = ruleGestureScores(geometry, true);
    assert(bestGesture(scores) == GestureClass::Pinch);
    assert(scores[gestureIndex(GestureClass::Pinch)] > 0.80F);
    assert(scores[gestureIndex(GestureClass::Fist)] < 0.50F);
}

void testPinchDepthNoiseTolerance()
{
    auto hand = makeHand(GestureClass::Pinch);
    // Tips still overlap in XY, but depth differs noticeably. Rule recognition
    // should not collapse because MediaPipe Z is noisier than image-space XY.
    hand.landmarks[4].z = 0.08F;
    hand.landmarks[8].z = -0.08F;

    const auto geometry = extractGestureGeometry(hand, 1000, 1000);
    assert(geometry.valid);
    const auto scores = ruleGestureScores(geometry, true);
    assert(bestGesture(scores) == GestureClass::Pinch);
}

void testPinchHysteresis()
{
    GestureEngine engine;
    TrackingFrame frame;
    frame.width = 1000;
    frame.height = 1000;
    frame.captureUs = 1'000'000;
    frame.sequence = 1;
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Pinch);

    GestureFrame result;
    for (int index = 0; index < 4; ++index) {
        frame.captureUs += 33'333;
        frame.sequence++;
        frame.hands[handIndex(HandSide::Right)].lastSeenUs = frame.captureUs;
        result = engine.update(frame, 70);
    }
    assert(result.hands[handIndex(HandSide::Right)].gesture == GestureClass::Pinch);

    // Move just outside the enter boundary but still inside the wider release
    // boundary. A latched pinch must remain stable.
    auto &hand = frame.hands[handIndex(HandSide::Right)];
    hand.landmarks[4].x = hand.landmarks[8].x - 0.050F;
    for (int index = 0; index < 3; ++index) {
        frame.captureUs += 33'333;
        frame.sequence++;
        hand.lastSeenUs = frame.captureUs;
        result = engine.update(frame, 70);
    }
    assert(result.hands[handIndex(HandSide::Right)].gesture == GestureClass::Pinch);

    // A clearly separated thumb/index must release the latch.
    hand.landmarks[4].x = hand.landmarks[8].x - 0.110F;
    for (int index = 0; index < 4; ++index) {
        frame.captureUs += 33'333;
        frame.sequence++;
        hand.lastSeenUs = frame.captureUs;
        result = engine.update(frame, 70);
    }
    assert(result.hands[handIndex(HandSide::Right)].gesture != GestureClass::Pinch);
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
    assert(stream.tensor().size() == 16U * kGestureFeatureCount);
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

    const auto rightFeatures = rightExtractor.update(right, 1000, 1000, 1'000'000);
    const auto leftFeatures = leftExtractor.update(left, 1000, 1000, 1'000'000);

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
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Fist);

    GestureFrame result;
    for (int index = 0; index < 6; ++index) {
        frame.captureUs += 33'333;
        frame.sequence++;
        frame.hands[handIndex(HandSide::Right)].lastSeenUs = frame.captureUs;
        result = engine.update(frame, 70);
    }

    const auto &right = result.hands[handIndex(HandSide::Right)];
    assert(right.ready);
    assert(right.gesture == GestureClass::Fist);
    assert(right.confidence >= 0.70F);

    frame.hands[handIndex(HandSide::Right)].tracked = false;
    frame.captureUs += 33'333;
    result = engine.update(frame, 70);

    const auto &lost = result.hands[handIndex(HandSide::Right)];
    assert(!lost.ready);
    assert(lost.gesture == GestureClass::None);
}

} // namespace

int main()
{
    testRegistryHasTwentyOneMappingGestures();
    testStaticRuleCatalog();
    testPinchBeatsFistWithCurledOtherFingers();
    testPinchDepthNoiseTolerance();
    testPinchHysteresis();
    testFeatureWindow();
    testLeftRightCanonicalization();
    testEngineThresholdAndReset();

    std::cout << "Gesture recognition tests passed\n";
    return 0;
}
