#include "gestures/ExperimentalStaticRecognizer.hpp"
#include "gestures/GestureEngine.hpp"
#include "gestures/GestureStateManager.hpp"
#include "gestures/GestureRegistry.hpp"
#include "mapping/MappingValidator.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <cstdlib>

namespace {
using namespace vc;

Landmark pt(float x, float y) { return {x, y, 0.0F}; }

void extended(std::array<Landmark, kHandLandmarkCount> &p,
              int base, float x, float y, float step)
{
    p[base] = pt(x, y);
    p[base + 1] = pt(x, y - step);
    p[base + 2] = pt(x, y - step * 2);
    p[base + 3] = pt(x, y - step * 3);
}

void curled(std::array<Landmark, kHandLandmarkCount> &p,
            int base, float x, float y)
{
    p[base] = pt(x, y);
    p[base + 1] = pt(x, y - 0.055F);
    p[base + 2] = pt(x + 0.050F, y - 0.025F);
    p[base + 3] = pt(x + 0.018F, y + 0.006F);
}

HandTrackingState makeHand(GestureClass gesture, HandSide side = HandSide::Right)
{
    HandTrackingState hand;
    hand.tracked = true;
    hand.side = side;
    hand.reportedSide = side;
    hand.handednessConfidence = 0.99F;
    hand.trackId = side == HandSide::Right ? 41 : 43;
    auto &p = hand.landmarks;
    p[0] = pt(0.50F, 0.82F);
    p[1] = pt(0.45F, 0.74F);
    p[2] = pt(0.39F, 0.68F);
    p[3] = pt(0.34F, 0.61F);
    p[4] = pt(0.30F, 0.54F);
    extended(p, 5, 0.41F, 0.65F, 0.075F);
    extended(p, 9, 0.50F, 0.61F, 0.082F);
    extended(p, 13, 0.59F, 0.65F, 0.073F);
    extended(p, 17, 0.67F, 0.69F, 0.062F);
    const bool foldThumb = gesture == GestureClass::Victory
        || gesture == GestureClass::Rock
        || gesture == GestureClass::ThreeFingers
        || gesture == GestureClass::FourFingers;
    if (foldThumb) {
        p[2] = pt(0.41F, 0.69F);
        p[3] = pt(0.46F, 0.70F);
        p[4] = pt(0.51F, 0.71F);
    }
    switch (gesture) {
    case GestureClass::Fist:
    case GestureClass::ThumbsUp:
    case GestureClass::ThumbsDown:
        for (const int base : {5, 9, 13, 17}) {
            const float x = base == 5 ? 0.41F : base == 9 ? 0.50F
                              : base == 13 ? 0.59F : 0.67F;
            const float y = base == 5 ? 0.65F : base == 9 ? 0.61F
                              : base == 13 ? 0.65F : 0.69F;
            curled(p, base, x, y);
        }
        if (gesture == GestureClass::ThumbsUp) {
            p[2] = pt(0.53F, 0.72F);
            p[3] = pt(0.53F, 0.58F);
            p[4] = pt(0.53F, 0.39F);
        } else if (gesture == GestureClass::ThumbsDown) {
            p[2] = pt(0.53F, 0.65F);
            p[3] = pt(0.53F, 0.82F);
            p[4] = pt(0.53F, 1.00F);
        }
        break;
    case GestureClass::Point:
        curled(p, 9, .50F, .61F);
        curled(p, 13, .59F, .65F);
        curled(p, 17, .67F, .69F);
        break;
    case GestureClass::Pinch:
        p[4] = pt(p[8].x - .008F, p[8].y + .006F);
        break;
    case GestureClass::Victory:
        curled(p, 13, .59F, .65F);
        curled(p, 17, .67F, .69F);
        break;
    case GestureClass::Rock:
        curled(p, 9, .50F, .61F);
        curled(p, 13, .59F, .65F);
        break;
    case GestureClass::ThreeFingers:
        curled(p, 17, .67F, .69F);
        break;
    case GestureClass::CallMe:
        curled(p, 5, .41F, .65F);
        curled(p, 9, .50F, .61F);
        curled(p, 13, .59F, .65F);
        break;
    case GestureClass::Ok:
        curled(p, 5, .41F, .65F);
        p[3] = pt(.39F, .63F);
        p[4] = pt(p[8].x + .005F, p[8].y + .004F);
        break;
    default:
        break;
    }
    if (side == HandSide::Left) {
        for (auto &mark : p)
            mark.x = 1.0F - mark.x;
    }
    return hand;
}

void testShapes()
{
    static_assert(kGestureClassCount == 5); // ONNX metadata is fixed
    static_assert(kStaticGestureCount == 13);
    const auto legacy = {GestureClass::Fist, GestureClass::OpenHand,
                         GestureClass::Point, GestureClass::Pinch};
    for (auto klass : legacy) {
        const auto g = extractGestureGeometry(makeHand(klass), 1000, 1000);
        const auto scores = experimentalStaticScores(g);
        assert(*std::max_element(scores.begin(), scores.end()) < 0.80F);
    }
    const auto additional = {
        GestureClass::ThumbsUp, GestureClass::ThumbsDown, GestureClass::Victory,
        GestureClass::Rock, GestureClass::ThreeFingers, GestureClass::FourFingers,
        GestureClass::CallMe, GestureClass::Ok
    };
    for (auto klass : additional) {
        assert(isRecognizedGesture(klass));
        assert(isAvailableGestureId(gestureCanonicalName(klass)));
        for (auto side : {HandSide::Left, HandSide::Right}) {
            for (const auto [width, height] : {
                     std::pair{1000, 1000}, std::pair{1280, 720}}) {
                const auto hand = makeHand(klass, side);
                const auto geometry = extractGestureGeometry(hand, width, height);
                assert(geometry.valid);
                const auto scores = experimentalStaticScores(geometry);
                const auto found = std::max_element(scores.begin(), scores.end());
                if (found == scores.end() || *found < 0.80F
                    || experimentalStaticClass(static_cast<std::size_t>(
                        std::distance(scores.begin(), found))) != klass) {
                    std::cerr << "Failed template " << gestureCanonicalName(klass)
                              << " on " << static_cast<int>(side) << " at "
                              << width << "x" << height << ", score "
                              << *found << "\n";
                    std::abort();
                }
            }
        }
    }
    assert(experimentalStaticClass(100) == GestureClass::None);
    assert(experimentalStaticScores({}) == ExperimentalStaticScores{});
    MappingProfile profile;
    profile.mappings.push_back({"test", HandSide::Left, GestureClass::Victory,
                                LogicalAction::KeySpace, ActionBehavior::Press, true});
    for (const auto &issue : validateMappings(profile))
        assert(issue.code != "unavailable_gesture");
}

void testEngineAndRelease()
{
    for (const auto klass : {GestureClass::ThumbsUp, GestureClass::Victory,
                             GestureClass::FourFingers, GestureClass::Ok}) {
        GestureEngine engine;
        GestureStateManager manager;
        GestureStateConfig config;
        config.debounceMs = 70;
        config.cooldownMs = 0;
        TrackingFrame frame;
        frame.width = frame.height = 1000;
        frame.hands[handIndex(HandSide::Right)] = makeHand(klass);
        GestureEventPhase lastPhase = GestureEventPhase::None;
        for (int i = 0; i < 16; ++i) {
            frame.captureUs = 1'000'000 + static_cast<std::int64_t>(i) * 33'333;
            frame.sequence = static_cast<std::uint64_t>(i + 1);
            const auto pred = engine.update(frame, 70);
            const auto events = manager.update(pred, config);
            lastPhase = events.hands[handIndex(HandSide::Right)].phase;
            if (i == 15) {
                assert(pred.hands[handIndex(HandSide::Right)].gesture == klass);
                assert(lastPhase == GestureEventPhase::Hold);
            }
        }
        frame.hands[handIndex(HandSide::Right)].tracked = false;
        frame.captureUs += 33'333;
        const auto events = manager.update(engine.update(frame, 70), config);
        assert(events.hands[handIndex(HandSide::Right)].phase
               == GestureEventPhase::Release);
        assert(events.hands[handIndex(HandSide::Right)].gesture == klass);
    }
}

void testExperimentalExitHysteresis()
{
    GestureStateManager manager;
    GestureStateConfig config;
    config.debounceMs = 0;
    config.cooldownMs = 0;
    config.dropoutGraceMs = 0;
    GestureFrame frame;
    frame.timestampUs = 1'000'000;
    auto &p = frame.hands[handIndex(HandSide::Right)];
    p.ready = true;
    p.tracked = true;
    p.trackId = 41;
    p.gesture = GestureClass::Victory;
    p.confidence = 0.95F;
    p.experimentalScores[gestureIndex(GestureClass::Victory) - kGestureClassCount] = .95F;
    manager.update(frame, config); // candidate
    frame.timestampUs += 33'333;
    auto evt = manager.update(frame, config).hands[handIndex(HandSide::Right)];
    assert(evt.gesture == GestureClass::Victory);
    assert(evt.phase == GestureEventPhase::Press);
    frame.timestampUs += 33'333;
    p.gesture = GestureClass::None;
    p.experimentalScores[gestureIndex(GestureClass::Victory) - kGestureClassCount] = .65F;
    evt = manager.update(frame, config).hands[handIndex(HandSide::Right)];
    assert(evt.phase == GestureEventPhase::Hold);
    frame.timestampUs += 33'333;
    p.tracked = false; // physical loss must NOT use the exit hysteresis
    evt = manager.update(frame, config).hands[handIndex(HandSide::Right)];
    assert(evt.phase == GestureEventPhase::Release);
    assert(evt.gesture == GestureClass::Victory);
}

void testModelWarmupContract()
{
    GestureEngine engine([](const std::vector<float> &) {
        GestureScores scores{};
        scores[0] = 1.0F;
        return scores;
    }, "fixed-legacy-five-classes");
    TrackingFrame frame;
    frame.width = frame.height = 1000;
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Rock);
    for (int i = 0; i < 34; ++i) {
        frame.captureUs = 1'000'000 + i * 33'333;
        frame.sequence = i + 1;
        const auto result = engine.update(frame, 70).hands[handIndex(HandSide::Right)];
        if (i < 15) {
            assert(!result.ready);
            assert(result.decision == GestureDecision::ModelWarmup);
        }
        if (i == 21) // new pose needs a stable window after model warmup
            assert(result.gesture == GestureClass::None);
        if (i == 33)
            assert(result.gesture == GestureClass::Rock);
    }
}

void testUncertainTransitionCannotClick()
{
    GestureEngine engine;
    GestureStateManager manager;
    GestureStateConfig config;
    config.debounceMs = 70;
    config.cooldownMs = 0;
    config.dropoutGraceMs = 0;

    TrackingFrame frame;
    frame.width = frame.height = 1000;
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Fist);
    for (int i = 0; i < 15; ++i) {
        frame.captureUs = 1'000'000 + i * 33'333;
        frame.sequence = i + 1;
        const auto prediction = engine.update(frame, 70);
        manager.update(prediction, config);
        if (i == 14)
            assert(prediction.hands[handIndex(HandSide::Right)].gesture
                   == GestureClass::Fist);
    }

    // When fingers open into VICTORY, do not emit the old FIST/POINT mapping
    // during the first uncertain transition frames.
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Victory);
    frame.captureUs += 33'333;
    auto prediction = engine.update(frame, 70);
    assert(prediction.hands[handIndex(HandSide::Right)].gesture == GestureClass::None);
    assert(prediction.hands[handIndex(HandSide::Right)].decision
           == GestureDecision::PoseStabilizing);
    assert(prediction.hands[handIndex(HandSide::Right)].experimentalTop
           == GestureClass::Victory);
    assert(prediction.hands[handIndex(HandSide::Right)].experimentalMargin >= 0.0F);
    const auto transitional = manager.update(prediction, config);
    assert(transitional.hands[handIndex(HandSide::Right)].phase
           == GestureEventPhase::Release);
    assert(transitional.hands[handIndex(HandSide::Right)].gesture
           == GestureClass::Fist);

    // One frame of the experimental pose is insufficient to activate it.
    frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Fist);
    frame.captureUs += 33'333;
    prediction = engine.update(frame, 70);
    assert(prediction.hands[handIndex(HandSide::Right)].gesture
           != GestureClass::Victory);

    // A camera stall invalidates the held experimental candidate.
    for (int i = 0; i < 18; ++i) {
        frame.hands[handIndex(HandSide::Right)] = makeHand(GestureClass::Victory);
        frame.captureUs += 33'333;
        engine.update(frame, 70);
    }
    frame.captureUs += kGestureMaxGapUs + 1000;
    prediction = engine.update(frame, 70);
    assert(prediction.hands[handIndex(HandSide::Right)].gesture
           == GestureClass::None);
    assert(prediction.hands[handIndex(HandSide::Right)].decision
           == GestureDecision::PoseStabilizing);
}
}

int main()
{
    testShapes();
    testEngineAndRelease();
    testExperimentalExitHysteresis();
    testModelWarmupContract();
    testUncertainTransitionCannotClick();
    std::cout << "Experimental static gestures verified\n";
}
