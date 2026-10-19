#include "runtime/GuiResultBackpressure.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

int main()
{
    auto gate = std::make_shared<vc::GuiResultBackpressure>();
    std::stop_source stop;
    const auto neverCancelled = [] { return false; };
    {
        // First delivery is pending; a second processed result may not queue.
        auto first = gate->acquire(stop.get_token(), neverCancelled);
        assert(first && gate->busy());
        std::atomic<bool> secondAcquired{false};
        std::jthread waiter([&](std::stop_token token) {
            auto second = gate->acquire(token, neverCancelled);
            secondAcquired.store(bool(second));
        });
        std::this_thread::sleep_for(30ms);
        assert(!secondAcquired.load());
        first.reset();  // Simulates Qt executing OR destroying a callback.
        waiter.join();
        assert(secondAcquired.load());
        assert(!gate->busy());
    }
    {
        auto first = gate->acquire(stop.get_token(), neverCancelled);
        assert(first);
        // Closing the camera session must interrupt the wait without deadlock.
        std::atomic<bool> closed{false};
        std::atomic<bool> cancelledWait{false};
        std::jthread waiter([&](std::stop_token token) {
            const auto second = gate->acquire(token, [&] { return closed.load(); });
            cancelledWait.store(!second);
        });
        std::this_thread::sleep_for(20ms);
        closed = true;
        waiter.join();
        assert(cancelledWait.load());
        first.reset();
    }
    {
        auto first = gate->acquire(stop.get_token(), neverCancelled);
        assert(first);
        std::stop_source cancel;
        std::atomic<bool> cancelledWait{false};
        std::jthread waiter([&](std::stop_token) {
            const auto second = gate->acquire(cancel.get_token(), neverCancelled);
            cancelledWait.store(!second);
        });
        std::this_thread::sleep_for(20ms);
        cancel.request_stop();
        waiter.join();
        assert(cancelledWait.load());
        first.reset();
    }
}
