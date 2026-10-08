#include "input/InputBackend.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vc {

#ifdef _WIN32
namespace {

WORD virtualKey(KeyCode key)
{
    switch (key) {
    case KeyCode::Space: return VK_SPACE;
    case KeyCode::Enter: return VK_RETURN;
    case KeyCode::Escape: return VK_ESCAPE;
    case KeyCode::Tab: return VK_TAB;
    case KeyCode::Left: return VK_LEFT;
    case KeyCode::Right: return VK_RIGHT;
    case KeyCode::Up: return VK_UP;
    case KeyCode::Down: return VK_DOWN;
    case KeyCode::Ctrl: return VK_CONTROL;
    case KeyCode::Shift: return VK_SHIFT;
    case KeyCode::Alt: return VK_MENU;
    case KeyCode::Count:
        break;
    }
    return 0;
}

bool isExtendedKey(KeyCode key)
{
    switch (key) {
    case KeyCode::Left:
    case KeyCode::Right:
    case KeyCode::Up:
    case KeyCode::Down:
        return true;
    default:
        return false;
    }
}

DWORD keyboardFlags(KeyCode key, bool keyUp)
{
    DWORD flags = keyUp ? KEYEVENTF_KEYUP : 0;
    if (isExtendedKey(key)) {
        flags |= KEYEVENTF_EXTENDEDKEY;
    }
    return flags;
}

class WindowsInputBackend final : public IInputBackend
{
public:
    ~WindowsInputBackend() override
    {
        releaseAll();
    }

    void apply(const ControllerState &state) override
    {
        residualX_ += state.mouseX;
        residualY_ += state.mouseY;
        residualWheel_ += state.wheel * 120.0F;

        const auto dx = static_cast<LONG>(residualX_);
        const auto dy = static_cast<LONG>(residualY_);
        const auto wheel = static_cast<LONG>(residualWheel_);

        residualX_ -= static_cast<float>(dx);
        residualY_ -= static_cast<float>(dy);
        residualWheel_ -= static_cast<float>(wheel);

        std::vector<INPUT> events;
        events.reserve(16);

        if (dx != 0 || dy != 0) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dx = dx;
            input.mi.dy = dy;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            events.push_back(input);
        }

        if (wheel != 0) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.mouseData = static_cast<DWORD>(wheel);
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            events.push_back(input);
        }

        if (state.mouseLeft != leftDown_) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = state.mouseLeft
                ? MOUSEEVENTF_LEFTDOWN
                : MOUSEEVENTF_LEFTUP;
            events.push_back(input);
        }

        if (state.mouseRight != rightDown_) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = state.mouseRight
                ? MOUSEEVENTF_RIGHTDOWN
                : MOUSEEVENTF_RIGHTUP;
            events.push_back(input);
        }

        const std::uint64_t changedKeys = state.keys ^ keysDown_;
        for (std::uint8_t index = 0;
             index < static_cast<std::uint8_t>(KeyCode::Count);
             ++index) {
            const std::uint64_t bit = std::uint64_t{1} << index;
            if ((changedKeys & bit) == 0) {
                continue;
            }

            const auto key = static_cast<KeyCode>(index);
            const WORD vk = virtualKey(key);
            if (vk == 0) {
                continue;
            }

            INPUT input{};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vk;
            input.ki.dwFlags = keyboardFlags(
                key,
                (state.keys & bit) == 0);
            events.push_back(input);
        }

        if (events.empty()) {
            leftDown_ = state.mouseLeft;
            rightDown_ = state.mouseRight;
            keysDown_ = state.keys;
            return;
        }

        const UINT sent = SendInput(
            static_cast<UINT>(events.size()),
            events.data(),
            sizeof(INPUT));

        if (sent != events.size()) {
            releaseAll();
            throw std::runtime_error(
                "SendInput failed. Windows may be blocking injected input "
                "(for example an elevated foreground application). Output "
                "was disarmed.");
        }

        leftDown_ = state.mouseLeft;
        rightDown_ = state.mouseRight;
        keysDown_ = state.keys;
    }

    void releaseAll() override
    {
        std::vector<INPUT> events;
        events.reserve(16);

        if (leftDown_) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            events.push_back(input);
        }

        if (rightDown_) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            events.push_back(input);
        }

        for (std::uint8_t index = 0;
             index < static_cast<std::uint8_t>(KeyCode::Count);
             ++index) {
            const std::uint64_t bit = std::uint64_t{1} << index;
            if ((keysDown_ & bit) == 0) {
                continue;
            }

            const WORD vk = virtualKey(static_cast<KeyCode>(index));
            if (vk == 0) {
                continue;
            }

            INPUT input{};
            input.type = INPUT_KEYBOARD;
            const auto key = static_cast<KeyCode>(index);
            input.ki.wVk = vk;
            input.ki.dwFlags = keyboardFlags(key, true);
            events.push_back(input);
        }

        if (!events.empty()) {
            SendInput(
                static_cast<UINT>(events.size()),
                events.data(),
                sizeof(INPUT));
        }

        leftDown_ = false;
        rightDown_ = false;
        keysDown_ = 0;
        residualX_ = 0.0F;
        residualY_ = 0.0F;
        residualWheel_ = 0.0F;
    }

private:
    bool leftDown_{};
    bool rightDown_{};
    std::uint64_t keysDown_{};
    float residualX_{};
    float residualY_{};
    float residualWheel_{};
};

} // namespace
#endif

std::unique_ptr<IInputBackend> makeSystemInputBackend()
{
#ifdef _WIN32
    return std::make_unique<WindowsInputBackend>();
#else
    throw std::runtime_error(
        "System mouse/keyboard output is only available on the Windows build.");
#endif
}

bool emergencyStopPressed()
{
#ifdef _WIN32
    return (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
#else
    return false;
#endif
}

} // namespace vc
