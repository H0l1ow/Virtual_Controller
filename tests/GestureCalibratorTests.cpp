#include "gestures/GestureCalibrator.hpp"
#include "gestures/GestureTuning.hpp"

#include <cassert>
#include <cstdint>

using namespace vc;

GestureFrame frame(int i, HandSide side, GestureClass selected,
                   float score, float rival, std::uint64_t identity = 7)
{
    GestureFrame result;
    result.timestampUs = 1'000'000 + 33'333LL * i;
    auto &p = result.hands[handIndex(side)];
    p.tracked = true;
    p.ready = true;
    p.geometryValid = true;
    p.trackId = identity;
    p.scores[gestureIndex(selected)] = score;
    p.scores[gestureIndex(selected == GestureClass::Fist
        ? GestureClass::Pinch : GestureClass::Fist)] = rival;
    return result;
}

void thresholds()
{
    GestureThresholds t;
    assert(t.forClass(GestureClass::Fist) == 80);
    assert(t.forClass(GestureClass::Pinch) == 80);
    t.fallbackPercent = 72;
    t.overrides[gestureIndex(GestureClass::Pinch)] = 62;
    assert(t.forClass(GestureClass::Fist) == 72);
    assert(t.forClass(GestureClass::Pinch) == 62);
    assert(t.forClass(GestureClass::None) == 72);
    t.overrides[gestureIndex(GestureClass::Fist)] = 115;
    assert(t.forClass(GestureClass::Fist) == 100);
}

void goodSamplesAndBothHands()
{
    GestureCalibrator c;
    c.start(HandSide::Right, GestureClass::Pinch);
    for (int i=0; i<GestureCalibrator::kRequiredSamples; ++i)
        c.ingest(frame(i, HandSide::Right, GestureClass::Pinch, .87F, .1F));
    assert(c.status() == GestureCalibrator::Status::Ready);
    assert(c.result(HandSide::Right, GestureClass::Pinch).valid);
    const int right = c.suggestedThreshold(GestureClass::Pinch);
    assert(right >= 70 && right <= 90);

    c.start(HandSide::Left, GestureClass::Pinch);
    for (int i=0; i<GestureCalibrator::kRequiredSamples; ++i)
        c.ingest(frame(i, HandSide::Left, GestureClass::Pinch, .72F, .12F));
    assert(c.status() == GestureCalibrator::Status::Ready);
    assert(c.result(HandSide::Left, GestureClass::Pinch).valid);
    assert(c.suggestedThreshold(GestureClass::Pinch) < right);
    // A failed retry for the weaker hand removes its old accepted sample,
    // but never erases a valid sample for the opposite hand.
    c.start(HandSide::Left, GestureClass::Pinch);
    assert(c.suggestedThreshold(GestureClass::Pinch) == right);
    for (int i=0; i<GestureCalibrator::kRequiredSamples; ++i)
        c.ingest(frame(i, HandSide::Left, GestureClass::Pinch, .45F, .1F));
    assert(c.status() == GestureCalibrator::Status::WeakSignal);
    assert(c.suggestedThreshold(GestureClass::Pinch) == right);
    c.clearResults();
    assert(c.suggestedThreshold(GestureClass::Pinch) == -1);
}

void rejectAmbiguousAndWeak()
{
    GestureCalibrator c;
    c.start(HandSide::Left, GestureClass::Fist);
    for (int i=0; i<GestureCalibrator::kRequiredSamples; ++i)
        c.ingest(frame(i, HandSide::Left, GestureClass::Fist, .48F, .10F));
    assert(c.status() == GestureCalibrator::Status::WeakSignal);
    assert(c.suggestedThreshold(GestureClass::Fist) == -1);
    c.start(HandSide::Left, GestureClass::Fist);
    for (int i=0; i<GestureCalibrator::kRequiredSamples; ++i)
        c.ingest(frame(i, HandSide::Left, GestureClass::Fist, .85F, .83F));
    assert(c.status() == GestureCalibrator::Status::Ambiguous);
    assert(c.suggestedThreshold(GestureClass::Fist) == -1);
}

void trackerIdentityAndTemporalGate()
{
    GestureCalibrator c;
    c.start(HandSide::Right, GestureClass::Point);
    auto first = frame(0, HandSide::Right, GestureClass::Point, .91F, .1F, 1);
    c.ingest(first);
    c.ingest(first);
    assert(c.sampleCount() == 1);
    auto second = frame(1, HandSide::Right, GestureClass::Point, .92F, .1F, 2);
    c.ingest(second);
    assert(c.sampleCount() == 1);
    c.cancel();
    assert(!c.capturing());
}

void ignoreExperimentalPoseDuringLegacyCalibration()
{
    GestureCalibrator c;
    c.start(HandSide::Right, GestureClass::Fist);
    auto simulated = frame(0, HandSide::Right, GestureClass::Fist, .95F, .1F);
    simulated.hands[handIndex(HandSide::Right)].gesture = GestureClass::ThumbsUp;
    c.ingest(simulated);
    assert(c.sampleCount() == 0);
    c.cancel();
    c.start(HandSide::Right, GestureClass::ThumbsUp);
    assert(!c.capturing()); // no per-class calibrator for experimental poses
    assert(c.suggestedThreshold(GestureClass::ThumbsUp) == -1);
}

int main()
{
    ignoreExperimentalPoseDuringLegacyCalibration();
    thresholds();
    goodSamplesAndBothHands();
    rejectAmbiguousAndWeak();
    trackerIdentityAndTemporalGate();
}
