#pragma once

#include "control/ControllerState.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace vc {

class IInputBackend
{
public:
    virtual ~IInputBackend() = default;
    virtual void apply(const ControllerState &state) = 0;
    virtual void releaseAll() = 0;
};

class MockInputBackend final : public IInputBackend
{
public:
    void apply(const ControllerState &state) override
    {
        last_ = state;
        ++applyCount_;
    }

    void releaseAll() override
    {
        last_ = {};
        ++releaseCount_;
    }

    [[nodiscard]] const ControllerState &last() const
    {
        return last_;
    }

    [[nodiscard]] std::uint64_t applyCount() const
    {
        return applyCount_;
    }

    [[nodiscard]] std::uint64_t releaseCount() const
    {
        return releaseCount_;
    }

private:
    ControllerState last_{};
    std::uint64_t applyCount_{};
    std::uint64_t releaseCount_{};
};

std::unique_ptr<IInputBackend> makeSystemInputBackend();
bool emergencyStopPressed();

} // namespace vc
