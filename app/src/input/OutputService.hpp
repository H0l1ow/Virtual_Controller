#pragma once

#include "control/ControllerState.hpp"
#include "input/InputBackend.hpp"
#include "input/OutputGuard.hpp"
#include "input/OutputResumePolicy.hpp"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace vc {

struct OutputStatus {
    bool ready{};
    bool armed{};
    bool requested{}; // User intent; survives transient tracking/watchdog failures.
    std::string reason; // Why an enabled output is temporarily suspended.
    std::string error;
    ControllerState applied{};
};

class OutputService final
{
public:
    enum class BackendKind {
        SystemMouse,
        Mock
    };

    explicit OutputService(
        BackendKind backendKind = BackendKind::SystemMouse);
    ~OutputService();

    OutputService(const OutputService &) = delete;
    OutputService &operator=(const OutputService &) = delete;

    bool arm();
    void stop(); // Explicit user OFF / STOP / F8: cancels auto-resume.
    void pause(const std::string &reason = "Waiting for fresh tracking data");

    void submit(
        const ControllerState &state,
        std::int64_t captureUs,
        std::uint64_t sequence,
        bool handTracked = true);

    [[nodiscard]] OutputStatus status() const;

private:
    void loop();
    std::unique_ptr<IInputBackend> createBackend() const;

    BackendKind backendKind_;

    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::thread thread_;
    bool quit_{};

    OutputGuard guard_;
    OutputResumePolicy resumePolicy_;
    std::int64_t retryAfterUs_{};
    // A held gesture present at reactivation must be released before its
    // button/key can generate a *new* press. Cursor motion stays available.
    bool suppressLeftUntilReleased_{};
    bool suppressRightUntilReleased_{};
    std::uint64_t suppressKeysUntilReleased_{};
    OutputStatus status_{};
};

} // namespace vc
