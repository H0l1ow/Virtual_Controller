#pragma once

#include "control/ControllerState.hpp"
#include "input/InputBackend.hpp"
#include "input/OutputGuard.hpp"

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
    void stop();

    void submit(
        const ControllerState &state,
        std::int64_t captureUs,
        std::uint64_t sequence);

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
    OutputStatus status_{};
};

} // namespace vc
