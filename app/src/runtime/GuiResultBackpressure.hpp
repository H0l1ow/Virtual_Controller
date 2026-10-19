#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <utility>

namespace vc {

// At most one processed frame can be queued to the Qt GUI at once.  The
// worker waits before processing the next (latest) camera frame, rather than
// posting an unbounded number of already classified gesture results.
//
// A lease is retained by the queued callback. Destroying the callback (even
// without executing it during QObject shutdown) releases the lease.  No
// already generated PRESS/RELEASE or recording sample is silently discarded.
class GuiResultBackpressure final : public std::enable_shared_from_this<GuiResultBackpressure>
{
public:
    class Lease final
    {
    public:
        explicit Lease(std::shared_ptr<GuiResultBackpressure> owner)
            : owner_(std::move(owner)) {}
        Lease(const Lease &) = delete;
        Lease &operator=(const Lease &) = delete;
        ~Lease() { owner_->release(); }

    private:
        std::shared_ptr<GuiResultBackpressure> owner_;
    };

    template<typename Cancelled>
    std::shared_ptr<Lease> acquire(std::stop_token stopToken, Cancelled cancelled)
    {
        std::unique_lock lock(mutex_);
        // A short timed wait also responds to session close() without needing
        // to share Qt's event loop or block the GUI thread on shutdown.
        while (busy_ && !stopToken.stop_requested() && !cancelled()) {
            available_.wait_for(lock, std::chrono::milliseconds(10));
        }
        if (stopToken.stop_requested() || cancelled())
            return {};
        busy_ = true;
        return std::make_shared<Lease>(shared_from_this());
    }

    bool busy() const
    {
        std::lock_guard lock(mutex_);
        return busy_;
    }

private:
    void release()
    {
        {
            std::lock_guard lock(mutex_);
            busy_ = false;
        }
        available_.notify_one();
    }

    mutable std::mutex mutex_;
    std::condition_variable available_;
    bool busy_{};
};

} // namespace vc
