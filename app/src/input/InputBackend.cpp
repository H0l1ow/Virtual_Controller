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

class WindowsMouseBackend final : public IInputBackend
{
public:
    ~WindowsMouseBackend() override
    {
        releaseAll();
    }

    void apply(const ControllerState &state) override
    {
        residualX_ += state.mouseX;
        residualY_ += state.mouseY;
        residualWheel_ += state.wheel * 120.0F;

        const auto dx =
            static_cast<LONG>(residualX_);
        const auto dy =
            static_cast<LONG>(residualY_);
        const auto wheel =
            static_cast<LONG>(residualWheel_);

        residualX_ -= static_cast<float>(dx);
        residualY_ -= static_cast<float>(dy);
        residualWheel_ -= static_cast<float>(wheel);

        std::vector<INPUT> events;
        events.reserve(4);

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
            input.mi.mouseData =
                static_cast<DWORD>(wheel);
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

        if (events.empty()) {
            leftDown_ = state.mouseLeft;
            rightDown_ = state.mouseRight;
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
    }

    void releaseAll() override
    {
        std::vector<INPUT> events;
        events.reserve(2);

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

        if (!events.empty()) {
            SendInput(
                static_cast<UINT>(events.size()),
                events.data(),
                sizeof(INPUT));
        }

        leftDown_ = false;
        rightDown_ = false;
        residualX_ = 0.0F;
        residualY_ = 0.0F;
        residualWheel_ = 0.0F;
    }

private:
    bool leftDown_{};
    bool rightDown_{};
    float residualX_{};
    float residualY_{};
    float residualWheel_{};
};

} // namespace
#endif

std::unique_ptr<IInputBackend> makeSystemMouseBackend()
{
#ifdef _WIN32
    return std::make_unique<WindowsMouseBackend>();
#else
    throw std::runtime_error(
        "System mouse output is only available on the Windows build.");
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
