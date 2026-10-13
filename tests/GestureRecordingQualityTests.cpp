#include "gestures/GestureDataRecorder.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace {
vc::TrackingFrame frame(std::int64_t timestamp, bool left, bool right)
{
    vc::TrackingFrame result{};
    result.width = 640;
    result.height = 480;
    result.captureUs = timestamp;
    for (std::size_t i = 0; i < 2; ++i) {
        auto &hand = result.hands[i];
        hand.tracked = i == 0 ? left : right;
        hand.trackId = hand.tracked ? i + 1 : 0;
        hand.handednessConfidence = 0.95F;
        for (std::size_t n = 0; n < vc::kHandLandmarkCount; ++n) {
            hand.landmarks[n] = {
                static_cast<float>(n % 5) / 20.0F,
                static_cast<float>(n / 5) / 20.0F, 0.01F};
        }
    }
    return result;
}
}
int main()
{
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "vc_m62_recording_stats";
    fs::remove_all(dir);
    vc::GestureDataRecorder recorder;
    vc::GestureDataRecorder::Labels labels{};
    labels[0] = vc::GestureClass::Pinch;
    assert(recorder.start(dir, labels));
    assert(recorder.labelled(vc::HandSide::Left));
    assert(!recorder.labelled(vc::HandSide::Right));
    assert(recorder.append(frame(1000, true, false)));
    assert(recorder.append(frame(2000, false, true)));
    assert(recorder.append(frame(3000, true, true)));
    assert(recorder.frameCount() == 3);
    assert(recorder.usableFrames(vc::HandSide::Left) == 2);
    assert(recorder.missingFrames(vc::HandSide::Left) == 1);
    assert(recorder.usableFrames(vc::HandSide::Right) == 0);
    assert(recorder.missingFrames(vc::HandSide::Right) == 0);
    recorder.stop();
    labels[0].reset();
    labels[1] = vc::GestureClass::None;
    assert(recorder.start(dir, labels));
    assert(recorder.frameCount() == 0);
    assert(recorder.usableFrames(vc::HandSide::Left) == 0);
    assert(recorder.usableFrames(vc::HandSide::Right) == 0);
    assert(recorder.append(frame(4000, false, true)));
    assert(recorder.usableFrames(vc::HandSide::Right) == 1);
    recorder.stop();
    fs::remove_all(dir);
    std::cout << "Recording quality accounting passed\n";
}
