#include "runtime/LatestValueMailbox.hpp"

#include <atomic>
#include <cassert>
#include <thread>

int main()
{
    vc::LatestValueMailbox<int> mailbox;
    assert(!mailbox.take());
    mailbox.publish(1);
    mailbox.publish(2);
    assert(mailbox.take().value() == 2);
    assert(!mailbox.take());
    mailbox.publish(3);
    mailbox.clear();
    assert(!mailbox.take());

    std::atomic_bool go{false};
    std::thread publisher([&] {
        while (!go.load(std::memory_order_acquire)) {}
        for (int n = 0; n < 10000; ++n) mailbox.publish(n);
        mailbox.publish(10000);
    });
    go.store(true, std::memory_order_release);
    publisher.join();
    assert(mailbox.take().value() == 10000);
    assert(!mailbox.take());
}
