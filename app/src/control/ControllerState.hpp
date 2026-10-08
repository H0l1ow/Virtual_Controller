#pragma once

#include <cmath>
#include <cstdint>

namespace vc {

struct ControlVector2 {
    float x{};
    float y{};
};

enum class KeyCode : std::uint8_t {
    Space = 0,
    Enter,
    Escape,
    Tab,
    Left,
    Right,
    Up,
    Down,
    Ctrl,
    Shift,
    Alt,
    Count
};

static_assert(
    static_cast<std::uint8_t>(KeyCode::Count) <= 64,
    "ControllerState key mask supports at most 64 logical keys");

constexpr std::uint64_t keyMask(KeyCode key)
{
    return std::uint64_t{1}
        << static_cast<std::uint8_t>(key);
}

// Backend-neutral logical output. Continuous controls and discrete gesture
// mappings both write into this state; OS/gamepad backends consume it without
// knowing how a gesture was recognized.
struct ControllerState {
    ControlVector2 left{};
    ControlVector2 right{};
    float lt{};
    float rt{};
    float mouseX{};
    float mouseY{};
    float wheel{};
    std::uint32_t buttons{};
    std::uint64_t keys{};
    bool mouseLeft{};
    bool mouseRight{};
    // Internal logical control directive. Input backends ignore this flag;
    // ActionMapper uses it to suppress relative cursor motion while tracking
    // continues to update the hand-motion baseline.
    bool cursorFrozen{};
    // Set by ActionMapper whenever no CursorMoveEnable mapping is currently
    // active. Cursor movement is denied by default even when system output is
    // armed. ContinuousControlInterpreter still advances its hand baseline so
    // opening the gate cannot cause a catch-up jump.
    bool cursorMovementLocked{};

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
            && keys == 0U
            && !mouseLeft
            && !mouseRight
            && !cursorFrozen;
    }
};

} // namespace vc
