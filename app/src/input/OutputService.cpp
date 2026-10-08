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

    if (!status_.ready) {
        return false;
    }

    guard_.arm();
    status_.armed = true;
    status_.error.clear();
    wake_.notify_all();
    return true;
}

void OutputService::stop()
{
    {
        std::lock_guard lock(mutex_);
        guard_.stop();
        status_.armed = false;
        status_.applied = {};
    }

    wake_.notify_all();
}

void OutputService::submit(
    const ControllerState &state,
    std::int64_t captureUs,
    std::uint64_t sequence)
{
    {
        std::lock_guard lock(mutex_);
        guard_.submit(
            state,
            captureUs,
            sequence);
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
                status_.error =
                    "F8 emergency stop: system output disarmed.";
            }

            const bool wasArmed = guard_.armed();
            const ControllerState state =
                guard_.poll(nowUs());

            if (wasArmed && !guard_.armed()) {
                status_.error =
                    "Output watchdog: tracking data became stale; "
                    "system output was disarmed.";
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

            // A transient SendInput failure (for example an elevated
            // foreground window) disarms output but does not destroy the
            // backend. The user can explicitly re-arm after the condition is
            // gone without restarting the application.
            status_.ready = true;
            status_.armed = false;
            status_.applied = {};
            status_.error = exception.what();
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
