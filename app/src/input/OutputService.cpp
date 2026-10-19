#include "input/OutputService.hpp"

#include "tracking/TrackingTypes.hpp"

#include <chrono>
#include <exception>

namespace vc {

OutputService::OutputService(
    BackendKind backendKind)
    : backendKind_(backendKind)
{
    thread_ = std::thread(
        [this] {
            loop();
        });
}

OutputService::~OutputService()
{
    {
        std::lock_guard lock(mutex_);
        quit_ = true;
        guard_.stop();
        status_.armed = false;
        status_.requested = false;
    }

    wake_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }
}

std::unique_ptr<IInputBackend> OutputService::createBackend() const
{
    if (backendKind_ == BackendKind::Mock) {
        return std::make_unique<MockInputBackend>();
    }

    return makeSystemInputBackend();
}

bool OutputService::arm()
{
    std::lock_guard lock(mutex_);
    if (!status_.ready)
        return false;

    // A user request is persistent. Physical output stays neutral until a
    // short sequence of fresh tracked frames has re-established confidence.
    guard_.stop();
    resumePolicy_.reset();
    suppressLeftUntilReleased_ = false;
    suppressRightUntilReleased_ = false;
    suppressKeysUntilReleased_ = 0;
    status_.requested = true;
    status_.armed = false;
    status_.applied = {};
    status_.reason = "Waiting for stable tracking";
    status_.error.clear();
    retryAfterUs_ = 0;
    wake_.notify_all();
    return true;
}

void OutputService::stop()
{
    {
        std::lock_guard lock(mutex_);
        guard_.stop();
        resumePolicy_.reset();
        suppressLeftUntilReleased_ = false;
        suppressRightUntilReleased_ = false;
        suppressKeysUntilReleased_ = 0;
        status_.requested = false;
        status_.armed = false;
        status_.applied = {};
        status_.reason.clear();
        status_.error.clear();
        retryAfterUs_ = 0;
    }
    wake_.notify_all();
}

void OutputService::pause(const std::string &reason)
{
    {
        std::lock_guard lock(mutex_);
        guard_.stop();
        resumePolicy_.reset();
        suppressLeftUntilReleased_ = false;
        suppressRightUntilReleased_ = false;
        suppressKeysUntilReleased_ = 0;
        status_.armed = false;
        status_.applied = {};
        if (status_.requested)
            status_.reason = reason;
    }
    wake_.notify_all();
}

void OutputService::submit(
    const ControllerState &state,
    std::int64_t captureUs,
    std::uint64_t sequence,
    bool handTracked)
{
    {
        std::lock_guard lock(mutex_);
        if (!status_.requested)
            return;

        const auto timestamp = nowUs();
        constexpr std::int64_t kMaxFrameAgeUs = 250'000;
        if (!handTracked || captureUs <= 0 || captureUs > timestamp
            || timestamp - captureUs > kMaxFrameAgeUs) {
            guard_.stop();
            resumePolicy_.reset();
            suppressLeftUntilReleased_ = false;
            suppressRightUntilReleased_ = false;
            suppressKeysUntilReleased_ = 0;
            status_.armed = false;
            status_.applied = {};
            status_.reason = "Tracking interrupted - waiting to resume";
        }
        else if (!guard_.armed()) {
            if (timestamp < retryAfterUs_)
                return;
            if (resumePolicy_.observe(captureUs, sequence, timestamp, handTracked)) {
                guard_.arm();
                // First frame after suspension is deliberately neutral. No
                // old mouse delta/held key is replayed by the resumed guard.
                guard_.submit({}, captureUs, sequence);
                suppressLeftUntilReleased_ = state.mouseLeft;
                suppressRightUntilReleased_ = state.mouseRight;
                suppressKeysUntilReleased_ = state.keys;
                status_.armed = true;
                status_.reason.clear();
                status_.error.clear();
            }
        }
        else {
            ControllerState safeState = state;
            if (suppressLeftUntilReleased_) {
                safeState.mouseLeft = false;
                if (!state.mouseLeft) suppressLeftUntilReleased_ = false;
            }
            if (suppressRightUntilReleased_) {
                safeState.mouseRight = false;
                if (!state.mouseRight) suppressRightUntilReleased_ = false;
            }
            safeState.keys &= ~suppressKeysUntilReleased_;
            suppressKeysUntilReleased_ &= state.keys;
            guard_.submit(safeState, captureUs, sequence);
        }
    }
    wake_.notify_all();
}

OutputStatus OutputService::status() const
{
    std::lock_guard lock(mutex_);
    return status_;
}

void OutputService::loop()
{
    std::unique_ptr<IInputBackend> backend;
    bool backendHadArmedOutput = false;

    try {
        backend = createBackend();

        std::lock_guard lock(mutex_);
        status_.ready = true;
        status_.error.clear();
    }
    catch (const std::exception &exception) {
        std::lock_guard lock(mutex_);
        status_.ready = false;
        status_.armed = false;
        status_.error = exception.what();
    }

    std::unique_lock lock(mutex_);

    while (!quit_) {
        if (!backend) {
            wake_.wait_for(
                lock,
                std::chrono::milliseconds(50));
            continue;
        }

        try {
            if (emergencyStopPressed()) {
                guard_.stop();
                resumePolicy_.reset();
                suppressLeftUntilReleased_ = false;
                suppressRightUntilReleased_ = false;
                suppressKeysUntilReleased_ = 0;
                status_.requested = false;
                status_.armed = false;
                status_.reason.clear();
                status_.error =
                    "F8 emergency stop: system output disarmed.";
            }

            const bool wasArmed = guard_.armed();
            const ControllerState state =
                guard_.poll(nowUs());

            if (wasArmed && !guard_.armed()) {
                resumePolicy_.reset();
                suppressLeftUntilReleased_ = false;
                suppressRightUntilReleased_ = false;
                suppressKeysUntilReleased_ = 0;
                if (status_.requested) {
                    status_.reason = "Tracking stalled - auto-resume pending";
                    status_.error.clear();
                }
            }

            if (guard_.armed()) {
                // Never hold the service mutex while calling OS input APIs.
                lock.unlock();
                backend->apply(state);
                lock.lock();

                status_.applied = state;
                backendHadArmedOutput = true;
            }
            else {
                if (backendHadArmedOutput) {
                    lock.unlock();
                    backend->releaseAll();
                    lock.lock();
                }

                status_.applied = {};
                backendHadArmedOutput = false;
            }

            status_.armed = guard_.armed();
        }
        catch (const std::exception &exception) {
            guard_.stop();

            lock.unlock();
            try {
                backend->releaseAll();
            }
            catch (...) {
            }
            lock.lock();

            // A transient SendInput failure must release all held inputs,
            // but must not silently erase the user's enabled preference.
            // Retry at a bounded rate; F8/explicit STOP still cancel intent.
            status_.ready = true;
            status_.armed = false;
            status_.applied = {};
            status_.reason = "Output backend error - retrying";
            status_.error = exception.what();
            retryAfterUs_ = nowUs() + 2'000'000;
            resumePolicy_.reset();
            suppressLeftUntilReleased_ = false;
            suppressRightUntilReleased_ = false;
            suppressKeysUntilReleased_ = 0;
            backendHadArmedOutput = false;
        }

        wake_.wait_for(
            lock,
            std::chrono::milliseconds(8));
    }

    lock.unlock();

    if (backend) {
        try {
            backend->releaseAll();
        }
        catch (...) {
        }
    }
}

} // namespace vc
