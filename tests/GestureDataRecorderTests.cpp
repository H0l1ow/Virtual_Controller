#include "gestures/GestureDataRecorder.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    namespace fs = std::filesystem;
    const auto folder = fs::temp_directory_path() / "vc_m5_recording_test";
    fs::remove_all(folder);
    vc::GestureDataRecorder recorder;
    vc::GestureDataRecorder::Labels labels{};
    labels[vc::handIndex(vc::HandSide::Left)] = vc::GestureClass::Pinch;
    labels[vc::handIndex(vc::HandSide::Right)] = vc::GestureClass::None;
    assert(recorder.start(folder, labels));
    assert(recorder.active());
    assert(!recorder.start(folder, labels));
    vc::TrackingFrame frame{};
    frame.width = 640;
    frame.height = 480;
    frame.captureUs = 1'000'000;
    frame.sequence = 17;
    auto &left = frame.hands[vc::handIndex(vc::HandSide::Left)];
    left.tracked = true;
    left.trackId = 9;
    left.reportedSide = vc::HandSide::Left;
    left.handednessConfidence = 0.93F;
    for (std::size_t i = 0; i < vc::kHandLandmarkCount; ++i) {
        left.landmarks[i] = {0.1F, static_cast<float>(i) / 100.0F, 0.0F};
    }
    assert(recorder.append(frame));
    assert(recorder.frameCount() == 1);
    recorder.stop();
    assert(!recorder.active());
    std::ifstream file(recorder.path());
    assert(file);
    std::string line;
    assert(std::getline(file, line));
    assert(line.find("\"schema\":\"vc.gestures.jsonl.v1\"") != std::string::npos);
    assert(line.find("\"label\":\"PINCH\"") != std::string::npos);
    assert(line.find("\"label\":\"NONE\"") != std::string::npos);
    assert(line.find("\"timestamp_us\":1000000") != std::string::npos);
    assert(line.find("\"landmarks\":[[0.100000001") != std::string::npos);
    assert(!std::getline(file, line));
    file.close();
    fs::remove_all(folder);
    std::cout << "Gesture recording tests passed\n";
}
