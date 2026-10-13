#include "gestures/HandRangeCalibrator.hpp"
#include <cassert>

int main()
{
    vc::HandRangeCalibrator cal;
    cal.start(vc::HandSide::Left, vc::HandRangeCalibrator::Kind::Motion);
    for (int i = 0; i < 60; ++i) {
        vc::TrackingFrame frame{};
        frame.captureUs = 1'000'000 + i * 35'000;
        frame.width = 640;
        frame.height = 480;
        auto &left = frame.hands[0];
        left.tracked = true;
        left.trackId = 42;
        left.palmPosition.x = 0.1F + 0.6F * i / 59.0F;
        left.palmPosition.y = 0.1F + 0.6F * i / 59.0F;
        cal.ingest(frame);
    }
    assert(!cal.capturing());
    assert(cal.status() == vc::HandRangeCalibrator::Status::Ready);
    const auto result = cal.result(vc::HandSide::Left);
    assert(result.motionValid && result.spanX > 0.55F && result.spanY > 0.55F);
    assert(!cal.result(vc::HandSide::Right).motionValid);
    cal.start(vc::HandSide::Right, vc::HandRangeCalibrator::Kind::Motion);
    for (int i = 0; i < 60; ++i) {
        vc::TrackingFrame frame{};
        frame.captureUs = 1'000'000 + i * 35'000;
        auto &right = frame.hands[1];
        right.tracked = true;
        right.trackId = 32;
        right.palmPosition.x = 0.5F;
        right.palmPosition.y = 0.5F;
        cal.ingest(frame);
    }
    assert(cal.status() == vc::HandRangeCalibrator::Status::Insufficient);
    assert(cal.result(vc::HandSide::Left).motionValid);
}
