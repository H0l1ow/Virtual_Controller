#pragma once

#include <mutex>
#include <optional>
#include <utility>

namespace vc {

// One pending value, never a FIFO. Publishing replaces obsolete data rather
// than making the UI replay old camera frames after a stall. This is a
// presentation-only mailbox; gesture events and dataset samples do NOT pass
// through it.
template<typename T>
class LatestValueMailbox final
{
public:
    void publish(T value)
    {
        std::lock_guard lock(mutex_);
        latest_ = std::move(value);
    }

    std::optional<T> take()
    {
        std::lock_guard lock(mutex_);
        auto result = std::move(latest_);
        latest_.reset();
        return result;
    }

    void clear()
    {
        std::lock_guard lock(mutex_);
        latest_.reset();
    }

private:
    std::mutex mutex_;
    std::optional<T> latest_;
};

} // namespace vc
