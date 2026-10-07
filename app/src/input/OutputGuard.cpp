#include "input/OutputGuard.hpp"

namespace vc {

OutputGuard::OutputGuard(
    std::int64_t timeoutUs)
    : timeoutUs_(timeoutUs)
{
}

void OutputGuard::arm()
{
    armed_ = true;
    requested_ = {};
    lastCaptureUs_ = -1;
    sequence_ = 0;
    consumedSequence_ = 0;
}

void OutputGuard::stop()
{
    armed_ = false;
    requested_ = {};
    lastCaptureUs_ = -1;
    sequence_ = 0;
    consumedSequence_ = 0;
}

void OutputGuard::submit(
    const ControllerState &state,
    std::int64_t captureUs,
    std::uint64_t sequence)
{
    if (!armed_) {
        return;
    }

    if (lastCaptureUs_ >= 0
        && (captureUs <= lastCaptureUs_
            || sequence <= sequence_)) {
        return;
    }

    requested_ = state;
    lastCaptureUs_ = captureUs;
    sequence_ = sequence;
}

ControllerState OutputGuard::poll(
    std::int64_t nowUs,
    bool allowed)
{
    if (!armed_) {
        return {};
    }

    if (!allowed
        || (lastCaptureUs_ >= 0
            && (nowUs < lastCaptureUs_
                || nowUs - lastCaptureUs_ > timeoutUs_))) {
        stop();
        return {};
    }

    if (lastCaptureUs_ < 0) {
        return {};
    }

    ControllerState result = requested_;

    // Relative mouse values are impulses: exactly one output cycle may consume
    // a given tracking frame. Held buttons remain present on later polls.
    if (sequence_ == consumedSequence_) {
        result.mouseX = 0.0F;
        result.mouseY = 0.0F;
        result.wheel = 0.0F;
    }

    consumedSequence_ = sequence_;
    return result;
}

} // namespace vc
