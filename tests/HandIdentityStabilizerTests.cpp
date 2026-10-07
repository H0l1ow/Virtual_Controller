#include "tracking/HandIdentityStabilizer.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace {

vc::RawHandDetection makeDetection(
    vc::HandSide side,
    float confidence,
    float x,
    float y = 0.5F)
{
    vc::RawHandDetection detection;
    detection.detected = true;
    detection.reportedSide = side;
    detection.handednessConfidence = confidence;

    for (auto &landmark : detection.landmarks) {
        landmark.x = x;
        landmark.y = y;
        landmark.z = 0.0F;
    }

    return detection;
}

vc::RawTrackingFrame makeFrame(
    std::int64_t captureUs,
    const vc::RawHandDetection &first,
    const vc::RawHandDetection &second = {})
{
    vc::RawTrackingFrame frame;
    frame.captureUs = captureUs;
    frame.trackStartUs = captureUs + 1000;
    frame.trackEndUs = captureUs + 5000;
    frame.width = 1280;
    frame.height = 720;
    frame.sequence = static_cast<std::uint64_t>(captureUs / 1000);
    frame.detections[0] = first;
    frame.detections[1] = second;
    return frame;
}

bool near(float a, float b)
{
    return std::abs(a - b) < 0.02F;
}

} // namespace

int main()
{
    vc::HandIdentityStabilizer stabilizer;

    const auto first = stabilizer.update(
        makeFrame(
            1'000'000,
            makeDetection(vc::HandSide::Left, 0.98F, 0.25F),
            makeDetection(vc::HandSide::Right, 0.97F, 0.75F)),
        false);

    assert(first.hands[0].tracked);
    assert(first.hands[1].tracked);
    assert(near(first.hands[0].palmPosition.x, 0.25F));
    assert(near(first.hands[1].palmPosition.x, 0.75F));

    const auto leftId = first.hands[0].trackId;
    const auto rightId = first.hands[1].trackId;

    // Deliberately flip MediaPipe handedness for one frame. Spatial history
    // should keep logical Left/Right identities stable.
    const auto mislabeled = stabilizer.update(
        makeFrame(
            1'033'000,
            makeDetection(vc::HandSide::Right, 0.75F, 0.28F),
            makeDetection(vc::HandSide::Left, 0.76F, 0.72F)),
        false);

    assert(mislabeled.hands[0].trackId == leftId);
    assert(mislabeled.hands[1].trackId == rightId);
    assert(near(mislabeled.hands[0].palmPosition.x, 0.28F));
    assert(near(mislabeled.hands[1].palmPosition.x, 0.72F));

    // Losing one hand must not make the other one swap identity.
    const auto oneHand = stabilizer.update(
        makeFrame(
            1'066'000,
            makeDetection(vc::HandSide::Right, 0.96F, 0.69F)),
        false);

    assert(!oneHand.hands[0].tracked);
    assert(oneHand.hands[1].tracked);
    assert(oneHand.hands[1].trackId == rightId);

    const auto reacquired = stabilizer.update(
        makeFrame(
            1'120'000,
            makeDetection(vc::HandSide::Left, 0.95F, 0.31F),
            makeDetection(vc::HandSide::Right, 0.95F, 0.67F)),
        false);

    assert(reacquired.hands[0].trackId == leftId);
    assert(reacquired.hands[1].trackId == rightId);

    // After the reacquisition window expires, a returning hand gets a new ID.
    stabilizer.update(
        makeFrame(1'600'000, {}),
        false);

    const auto newLeft = stabilizer.update(
        makeFrame(
            1'650'000,
            makeDetection(vc::HandSide::Left, 0.99F, 0.32F)),
        false);

    assert(newLeft.hands[0].tracked);
    assert(newLeft.hands[0].trackId != leftId);

    stabilizer.reset();

    const auto swapped = stabilizer.update(
        makeFrame(
            2'000'000,
            makeDetection(vc::HandSide::Left, 0.99F, 0.20F)),
        true);

    assert(!swapped.hands[0].tracked);
    assert(swapped.hands[1].tracked);

    return 0;
}
