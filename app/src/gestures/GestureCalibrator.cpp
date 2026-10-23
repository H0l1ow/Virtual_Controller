#include "gestures/GestureCalibrator.hpp"

#include <algorithm>
#include <cmath>

namespace vc {

void GestureCalibrator::start(HandSide side, GestureClass gesture)
{
    cancel();
    if (gesture == GestureClass::None || gestureIndex(gesture) >= kGestureClassCount)
        return;
    side_ = side;
    gesture_ = gesture;
    // A failed retry must not leave a stale accepted suggestion for this hand.
    results_[handIndex(side)][gestureIndex(gesture)] = {};
    status_ = Status::Capturing;
    samples_.reserve(kRequiredSamples);
}

void GestureCalibrator::cancel()
{
    if (capturing())
        status_ = Status::Cancelled;
    samples_.clear();
    activeTrackId_ = 0;
    lastSampleUs_ = 0;
}

void GestureCalibrator::clearResults()
{
    cancel();
    results_ = {};
    status_ = Status::Idle;
}

void GestureCalibrator::ingest(const GestureFrame &frame)
{
    if (!capturing() || frame.timestampUs <= 0)
        return;

    const auto &p = frame.hands[handIndex(side_)];
    if (!p.tracked || !p.geometryValid || !p.ready || !p.trackId)
        return;
    // Prevent a THUMBS_UP pose (which still has a high legacy FIST score)
    // from contaminating a five-class threshold calibration session.
    if (gestureIndex(p.gesture) >= kGestureClassCount)
        return;

    // Do not combine measurements from two different tracked identities.
    if (activeTrackId_ != 0 && activeTrackId_ != p.trackId) {
        samples_.clear();
        lastSampleUs_ = 0;
    }
    activeTrackId_ = p.trackId;

    // Limit repeated timestamps / high-FPS camera bias without depending on
    // how quickly the QML view can render diagnostic values.
    if (lastSampleUs_ && frame.timestampUs - lastSampleUs_ < 30000)
        return;
    if (lastSampleUs_ && frame.timestampUs <= lastSampleUs_)
        return;
    lastSampleUs_ = frame.timestampUs;

    const auto idx = gestureIndex(gesture_);
    const float score = p.scores[idx];
    if (!std::isfinite(score) || score < 0.0F || score > 1.0F)
        return;

    float competitor = 0.0F;
    for (std::size_t i = 1; i < p.scores.size(); ++i) {
        if (i != idx && std::isfinite(p.scores[i]))
            competitor = std::max(competitor, p.scores[i]);
    }
    samples_.push_back({score, competitor});
    if (sampleCount() >= kRequiredSamples)
        finalize();
}

void GestureCalibrator::finalize()
{
    if (sampleCount() != kRequiredSamples)
        return;

    std::vector<float> expected;
    expected.reserve(samples_.size());
    int dominant = 0;
    for (const auto &sample : samples_) {
        expected.push_back(sample.expected);
        if (sample.expected > sample.competing + 0.05F)
            ++dominant;
    }
    std::sort(expected.begin(), expected.end());
    const float median = expected[expected.size() / 2];
    const int dominance = 100 * dominant / kRequiredSamples;

    if (median < 0.55F) {
        status_ = Status::WeakSignal;
        return;
    }
    if (dominance < 75) {
        status_ = Status::Ambiguous;
        return;
    }

    // 20th percentile - 5 points of tolerance; minimum 60 avoids making
    // a pose too easy to trigger merely from a few weak examples.
    const auto lower = expected[expected.size() / 5];
    const int threshold = std::clamp(
        static_cast<int>(std::floor(lower * 100.0F)) - 5,
        60, 95);
    results_[handIndex(side_)][gestureIndex(gesture_)] = {
        true, threshold, kRequiredSamples, dominance
    };
    status_ = Status::Ready;
}

GestureCalibrator::Result GestureCalibrator::result(
    HandSide side, GestureClass gesture) const
{
    if (gestureIndex(gesture) >= kGestureClassCount)
        return {};
    return results_[handIndex(side)][gestureIndex(gesture)];
}

int GestureCalibrator::suggestedThreshold(GestureClass gesture) const
{
    if (gestureIndex(gesture) >= kGestureClassCount)
        return -1;
    int candidate = -1;
    for (std::size_t side = 0; side < kHandCount; ++side) {
        const auto &r = results_[side][gestureIndex(gesture)];
        if (r.valid)
            candidate = candidate < 0 ? r.threshold : std::min(candidate, r.threshold);
    }
    return candidate;
}

} // namespace vc
