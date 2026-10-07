#pragma once

#include "control/ControllerState.hpp"

#include <cstdint>

namespace vc {

class OutputGuard final
{
public:
    explicit OutputGuard(
        std::int64_t timeoutUs = 250000);

    void arm();
    void stop();

    void submit(
        const ControllerState &state,
        std::int64_t captureUs,
        std::uint64_t sequence);

    ControllerState poll(
        std::int64_t nowUs,
        bool allowed = true);

    [[nodiscard]] bool armed() const
    {
        return armed_;
    }

private:
    std::int64_t timeoutUs_{};
    std::int64_t lastCaptureUs_{-1};
    ControllerState requested_{};
    bool armed_{};
    std::uint64_t sequence_{};
    std::uint64_t consumedSequence_{};
};

} // namespace vc
