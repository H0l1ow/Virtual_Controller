#include "runtime/PreviewFrameLimiter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    vc::PreviewFrameLimiter limiter(12);
    assert(!limiter.shouldForward(0));
    assert(limiter.shouldForward(1'000'000));
    assert(!limiter.shouldForward(1'010'000));
    assert(!limiter.shouldForward(1'080'000));
    assert(limiter.shouldForward(1'084'000));

    limiter.reset();
    // 60 source FPS for ~2 seconds; the secondary preview forwards <=12 FPS.
    int forwarded = 0;
    for (std::int64_t i = 0; i < 120; ++i) {
        if (limiter.shouldForward(1'000'000 + i * 16'667)) ++forwarded;
    }
    assert(forwarded >= 22 && forwarded <= 25);
    // At an actual ~30 FPS camera rate, use a threshold that never
    // accidentally halves the frame rate due to 33.0 vs 33.3 ms jitter.
    vc::PreviewFrameLimiter controllerLimiter(45);
    int primaryFrames = 0;
    for (std::int64_t i = 0; i < 30; ++i) {
        if (controllerLimiter.shouldForward(1'000'000 + i * 33'000))
            ++primaryFrames;
    }
    assert(primaryFrames == 30);
    // A frame arriving after disconnect/reconnect is displayed immediately.
    limiter.reset();
    assert(limiter.shouldForward(2'000'000));
    // A backward time jump cannot permanently block future frames.
    assert(limiter.shouldForward(500'000));
    assert(limiter.shouldForward(600'000));
    std::cout << "Secondary preview rate limiter passed\n";
}
