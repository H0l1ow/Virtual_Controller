#pragma once

#include <cmath>
#include <cstdint>

namespace vc {

struct ControlVector2 {
    float x{};
    float y{};
};

// Backend-neutral logical output. M2 currently drives only mouseX/mouseY,
// while the remaining fields reserve the same contract for later gamepad and
// gesture milestones.
struct ControllerState {
    ControlVector2 left{};
    ControlVector2 right{};
    float lt{};
    float rt{};
    float mouseX{};
    float mouseY{};
    float wheel{};
    std::uint32_t buttons{};
    bool mouseLeft{};
    bool mouseRight{};

    [[nodiscard]] bool neutral(float epsilon = 0.0001F) const
    {
        const auto zero = [epsilon](float value) {
            return std::abs(value) <= epsilon;
        };

        return zero(left.x)
            && zero(left.y)
            && zero(right.x)
            && zero(right.y)
            && zero(lt)
            && zero(rt)
            && zero(mouseX)
            && zero(mouseY)
            && zero(wheel)
            && buttons == 0U
            && !mouseLeft
            && !mouseRight;
    }
};

} // namespace vc
