#include "gestures/GestureDataRecorder.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace vc {

namespace {

std::string timestampFileName()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream buffer;
    buffer << "gesture_session_" << std::put_time(&utc, "%Y%m%d_%H%M%S")
        << '_' << std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()).count() % 1000
        << ".jsonl";
    return buffer.str();
}

const char *handName(HandSide side)
{
    return side == HandSide::Left ? "LEFT" : "RIGHT";
}

} // namespace

bool GestureDataRecorder::start(
    const std::filesystem::path &directory, Labels labels)
{
    if (active()) {
        error_ = "Recording already running";
        return false;
    }
    error_.clear();
    path_.clear();
    frameCount_ = 0;
    labels_ = labels;

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        error_ = "Cannot create recording directory: " + ec.message();
        return false;
    }

    // Guard against file-name collisions after rapid restarts.
    const auto name = timestampFileName();
    const auto stem = name.substr(0, name.size() - 6); // remove .jsonl
    path_ = directory / name;
    for (int suffix = 1; std::filesystem::exists(path_, ec) && suffix < 100;
         ++suffix) {
        path_ = directory / (stem + "_" + std::to_string(suffix) + ".jsonl");
    }
    if (ec || std::filesystem::exists(path_, ec)) {
        error_ = "Cannot create a unique recording file";
        return false;
    }

    stream_.open(path_, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!stream_.is_open()) {
        error_ = "Cannot open recording file for writing";
        return false;
    }
    return true;
}

bool GestureDataRecorder::append(const TrackingFrame &frame)
{
    if (!active()) {
        return false;
    }
    if (frameCount_ >= kMaxFrames) {
        stop();
        error_ = "Recording stopped at 18000 frames (session size limit)";
        return false;
    }

    std::ostringstream line;
    line << std::setprecision(9);
    line << "{\"schema\":\"vc.gestures.jsonl.v1\",\"sequence\":"
         << frame.sequence
         << ",\"timestamp_us\":" << frame.captureUs
         << ",\"width\":" << frame.width
         << ",\"height\":" << frame.height
         << ",\"hands\":[";
    for (std::size_t side = 0; side < kHandCount; ++side) {
        if (side != 0) {
            line << ',';
        }
        const auto &hand = frame.hands[side];
        line << "{\"side\":\""
             << handName(static_cast<HandSide>(side))
             << "\",\"reported_side\":\""
             << handName(hand.reportedSide)
             << "\",\"tracked\":" << (hand.tracked ? "true" : "false")
             << ",\"track_id\":" << (hand.tracked ? hand.trackId : 0)
             << ",\"handedness_confidence\":"
             << (hand.tracked && std::isfinite(hand.handednessConfidence)
                     ? hand.handednessConfidence : 0.0F)
             << ",\"label\":\""
             << (labels_[side] ? gestureCanonicalName(*labels_[side]) : "UNLABELED")
             << "\",\"landmarks\":[";
        if (hand.tracked) {
            for (std::size_t i = 0; i < kHandLandmarkCount; ++i) {
                if (i != 0) {
                    line << ',';
                }
                const auto &point = hand.landmarks[i];
                // Malformed samples remain valid JSON and are distinguishable
                // from valid coordinates by null values.
                line << '[';
                if (std::isfinite(point.x)) line << point.x; else line << "null";
                line << ',';
                if (std::isfinite(point.y)) line << point.y; else line << "null";
                line << ',';
                if (std::isfinite(point.z)) line << point.z; else line << "null";
                line << ']';
            }
        }
        line << "]}";
    }
    line << "]}\n";
    stream_ << line.str();
    if (!stream_) {
        stop();
        error_ = "Write failed: check storage space and permissions";
        return false;
    }
    ++frameCount_;
    return true;
}

void GestureDataRecorder::stop()
{
    if (stream_.is_open()) {
        stream_.flush();
        if (!stream_) {
            error_ = "Failed to flush recording to disk";
        }
        stream_.close();
    }
}

} // namespace vc
