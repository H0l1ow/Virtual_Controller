#include "gestures/HandRangeCalibrator.hpp"

#include <algorithm>
#include <cmath>

namespace vc {

void HandRangeCalibrator::start(HandSide side, Kind kind)
{
    cancel();
    side_ = side;
    kind_ = kind;
    auto &result = results_[handIndex(side)];
    if (kind == Kind::Motion) result.motionValid = false;
    else if (kind == Kind::PinchClosed) result.closedValid = false;
    else result.openValid = false;
    status_ = Status::Capturing;
    samples_ = 0;
    trackId_ = 0;
    previousUs_ = 0;
    minX_ = minY_ = 1.0F;
    maxX_ = maxY_ = 0.0F;
    pinchValues_.clear();
}

void HandRangeCalibrator::ingest(const TrackingFrame &frame)
{
    if (!capturing() || frame.captureUs <= 0) return;
    const auto &hand = frame.hands[handIndex(side_)];
    if (!hand.tracked || hand.trackId == 0 || !std::isfinite(hand.palmPosition.x)
        || !std::isfinite(hand.palmPosition.y)) return;
    if (trackId_ != 0 && trackId_ != hand.trackId) {
        // Never combine measurements from different hands/tracking identities.
        start(side_, kind_);
    }
    trackId_ = hand.trackId;
    if (frame.captureUs <= previousUs_ || (previousUs_ > 0 && frame.captureUs - previousUs_ < 30'000))
        return;
    previousUs_ = frame.captureUs;
    if (kind_ == Kind::Motion) {
        const float x = hand.palmPosition.x, y = hand.palmPosition.y;
        if (x < 0.0F || x > 1.0F || y < 0.0F || y > 1.0F) return;
        minX_ = std::min(minX_, x);
        maxX_ = std::max(maxX_, x);
        minY_ = std::min(minY_, y);
        maxY_ = std::max(maxY_, y);
    } else {
        const auto geometry = extractGestureGeometry(hand, frame.width, frame.height);
        if (!geometry.valid || !std::isfinite(geometry.pinch[0])) return;
        pinchValues_.push_back(geometry.pinch[0]);
    }
    if (++samples_ < kSamples) return;
    auto &result = results_[handIndex(side_)];
    if (kind_ == Kind::Motion) {
        const float spanX = maxX_ - minX_;
        const float spanY = maxY_ - minY_;
        if (spanX < 0.12F || spanY < 0.12F) {
            status_ = Status::Insufficient;
            return;
        }
        result.motionValid = true;
        result.spanX = spanX;
        result.spanY = spanY;
    } else {
        std::sort(pinchValues_.begin(), pinchValues_.end());
        const float median = pinchValues_[pinchValues_.size() / 2];
        if (median < 0.02F || median > 3.0F) {
            status_ = Status::Insufficient;
            return;
        }
        if (kind_ == Kind::PinchClosed) {
            result.closedValid = true;
            result.closedDistance = median;
        } else {
            result.openValid = true;
            result.openDistance = median;
        }
    }
    status_ = Status::Ready;
}

void HandRangeCalibrator::cancel()
{
    if (capturing()) status_ = Status::Cancelled;
}

void HandRangeCalibrator::reset()
{
    cancel();
    results_ = {};
    status_ = Status::Idle;
}

} // namespace vc
