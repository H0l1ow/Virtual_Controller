#pragma once

#include <string>
#include <utility>

namespace vc {

// Bounded one-shot restart after a camera disappears. The UI/runtime owns the
// timer and must check the *original* device id, not a fallback camera.
class CameraReconnectPolicy final {
public:
    enum class Decision { Idle, Wait, Restart, TimedOut };

    bool begin(std::string deviceId, int maxPolls = 30)
    {
        if (deviceId.empty() || maxPolls < 1)
            return false;
        deviceId_ = std::move(deviceId);
        maxPolls_ = maxPolls;
        polls_ = 0;
        pending_ = true;
        return true;
    }

    void cancel()
    {
        pending_ = false;
        deviceId_.clear();
        polls_ = 0;
    }

    bool pending() const { return pending_; }
    const std::string &deviceId() const { return deviceId_; }

    Decision poll(bool originalDevicePresent, bool stopped)
    {
        if (!pending_)
            return Decision::Idle;

        // Check reconnect before timeout: a camera returning on the final
        // allowed poll must still be recoverable.
        if (originalDevicePresent && stopped) {
            cancel();
            return Decision::Restart;
        }
        if (++polls_ >= maxPolls_) {
            cancel();
            return Decision::TimedOut;
        }
        return Decision::Wait;
    }

private:
    std::string deviceId_;
    int maxPolls_{30};
    int polls_{};
    bool pending_{};
};

} // namespace vc
