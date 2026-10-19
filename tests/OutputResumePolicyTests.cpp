#include "input/OutputResumePolicy.hpp"

#include <cassert>
#include <cstdint>

int main()
{
    vc::OutputResumePolicy policy;
    const std::int64_t start = 1'000'000;

    // Tracking disappears: no arming, even if frames continue arriving.
    for (unsigned i = 0; i < 20; ++i)
        assert(!policy.observe(start + i * 40'000, i + 1,
                               start + i * 40'000, false));

    // A valid hand must remain present for at least 120 ms across >=4 frames.
    assert(!policy.observe(2'000'000, 21, 2'000'000, true));
    assert(!policy.observe(2'040'000, 22, 2'040'000, true));
    assert(!policy.observe(2'080'000, 23, 2'080'000, true));
    assert(policy.observe(2'120'000, 24, 2'120'000, true));

    policy.reset();
    assert(!policy.observe(3'000'000, 30, 3'000'000, true));
    // Duplicate and out-of-order frames cannot count towards recovery.
    assert(!policy.observe(3'000'000, 30, 3'010'000, true));
    assert(!policy.observe(3'030'000, 31, 3'030'000, true));
    // Camera freeze resets the stable streak.
    assert(!policy.observe(3'400'000, 32, 3'400'000, true));
    // Excess GUI delay makes a frame stale and restarts the streak.
    assert(!policy.observe(3'440'000, 33, 3'800'000, true));
    assert(!policy.observe(3'840'000, 34, 3'840'000, true));
    assert(!policy.observe(3'880'000, 35, 3'880'000, true));
    assert(!policy.observe(3'920'000, 36, 3'920'000, true));
    assert(policy.observe(3'960'000, 37, 3'960'000, true));
    // A frame from the future must not pass freshness checks.
    policy.reset();
    assert(!policy.observe(4'010'000, 38, 4'000'000, true));
    return 0;
}
